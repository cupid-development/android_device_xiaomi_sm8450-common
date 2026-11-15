# SPDX-FileCopyrightText: 2026 The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
"""Execute stock ARM64 math without loading its Android services.

The allocator, logging, vector getters and reporting/filtering are stubbed.
Neural inference, channel subtraction and lux conversion execute stock instructions.
Requires the optional Python 'unicorn' package; never used by the product build.
"""

import struct
import math
import subprocess
import re
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from extract_model import Elf, SUPPORTED
from unicorn import Uc, UC_ARCH_ARM64, UC_MODE_ARM, UC_HOOK_CODE
from unicorn.arm64_const import (
    UC_ARM64_REG_D0,
    UC_ARM64_REG_LR,
    UC_ARM64_REG_PC,
    UC_ARM64_REG_SP,
    UC_ARM64_REG_TPIDR_EL0,
    UC_ARM64_REG_X0,
    UC_ARM64_REG_X1,
    UC_ARM64_REG_X2,
    UC_ARM64_REG_X8,
)


class Reference:
    def __init__(
        self,
        device,
        dumps,
        objdump,
        scale=(1.0, 1.0, 1.0, 1.0),
        ratio=(1.0, 1.0, 1.0, 1.0),
    ):
        self.device = device
        self.scale, self.ratio = scale, ratio
        self.path = str(
            Path(dumps)
            / device
            / "vendor/lib64/hw/vendor.xiaomi.sensor.citsensorservice@2.0-impl.so"
        )
        e = Elf(self.path)
        assert e.sha256 == SUPPORTED[device], "Unrecognized stock reference binary"
        self.u = Uc(UC_ARCH_ARM64, UC_MODE_ARM)
        u = self.u
        u.mem_map(0, 0x200000)
        for address, offset, size in e.segments:
            u.mem_write(address, e.data[offset : offset + size])
        u.mem_map(0x10000000, 0x200000)
        u.mem_map(0x20000000, 0x1000)
        self.heap = 0x10100000
        self.symbols = {
            int(a, 16): n
            for a, n in re.findall(
                r"^([0-9a-f]+) <(.+)>:",
                subprocess.check_output(
                    [str(objdump), "-d", "--section=.plt", "--demangle", self.path],
                    text=True,
                ),
                re.M,
            )
        }
        u.hook_add(UC_HOOK_CODE, self.hook)

    def q(self, a):
        return struct.unpack("<Q", self.u.mem_read(a, 8))[0]

    def doubles(self, a, n):
        return struct.unpack("<" + "d" * n, self.u.mem_read(a, n * 8))

    def alloc(self, data):
        a = self.heap
        self.heap += (len(data) + 15) & ~15
        self.u.mem_write(a, data)
        return a

    def vec(self, dst, values):
        a = self.alloc(struct.pack("<" + "d" * len(values), *values))
        self.u.mem_write(
            dst, struct.pack("<QQQ", a, a + len(values) * 8, a + len(values) * 8)
        )

    def hook(self, u, pc, size, _):
        name = self.symbols.get(pc)
        if name is None:
            return
        x0 = u.reg_read(UC_ARM64_REG_X0)
        x1 = u.reg_read(UC_ARM64_REG_X1)
        x2 = u.reg_read(UC_ARM64_REG_X2)
        x8 = u.reg_read(UC_ARM64_REG_X8)

        def getd(n):
            return struct.unpack(
                "<d", struct.pack("<Q", u.reg_read(UC_ARM64_REG_D0 + n))
            )[0]

        def setd(v):
            u.reg_write(UC_ARM64_REG_D0, struct.unpack("<Q", struct.pack("<d", v))[0])

        if name == "memset@plt":
            u.mem_write(x0, bytes([x1 & 255]) * x2)
        elif name == "pow@plt":
            setd(math.pow(getd(0), getd(1)))
        elif name == "log@plt":
            setd(math.log(getd(0)))
        elif name.startswith("operator new(") or name == "_Znwm@plt":
            u.reg_write(UC_ARM64_REG_X0, self.alloc(bytes(x0)))
        elif name.startswith("operator delete(") or name == "_ZdlPv@plt":
            pass
        elif name == "__android_log_print@plt":
            u.reg_write(UC_ARM64_REG_X0, 0)
        elif "getChannelCoef" in name:
            self.vec(
                x8,
                [424.993, 8.712, -5.631, 5.99, -10.11]
                if self.device == "marble"
                else [
                    0.06,
                    0.089700,
                    -0.054771,
                    0.030031,
                    -0.145131,
                    -0.089847,
                    0.058323,
                    0.515587,
                    -0.389230,
                ],
            )
        elif "getScale" in name:
            self.vec(x8, self.scale)
        elif "getChannelDataCalibrated" in name or "getDcChannelDataCalibrated" in name:
            self.bleed = tuple(
                v * r for v, r in zip(self.doubles(self.q(x2), 4), self.ratio)
            )
            self.vec(x8, self.bleed)
        elif "updateArray" in name:
            self.lux = getd(0)
        elif "lowLuxStableFilter" in name:
            pass
        elif "RGB_test_" in name:
            target = next(
                a
                for n, a in {
                    "0527": 0x95534,
                    "0507": 0x950A4,
                    "0407_900": 0x94780,
                    "0407_2047": 0x94C10,
                }.items()
                if n in name
            )
            u.reg_write(UC_ARM64_REG_PC, target)
            return
        else:
            raise RuntimeError((hex(pc), name))
        u.reg_write(UC_ARM64_REG_PC, u.reg_read(UC_ARM64_REG_LR))

    def run(
        self,
        inputs,
        index=0,
        raw=(10000.0, 10000.0, 10000.0, 10000.0),
        full=False,
        dc=False,
    ):
        self.heap = 0x10100000
        u = self.u
        u.reg_write(UC_ARM64_REG_SP, 0x100F0000)
        u.reg_write(UC_ARM64_REG_TPIDR_EL0, 0x10000000)
        u.reg_write(UC_ARM64_REG_LR, 0x20000000)
        data = self.alloc(struct.pack("<4d", *inputs))
        out = self.alloc(bytes(32))
        if self.device == "mayfly" and not full:
            u.reg_write(UC_ARM64_REG_X0, data)
            u.reg_write(UC_ARM64_REG_X1, out)
            address = [0x95534, 0x950A4, 0x94780, 0x94C10][index]
        else:
            obj = self.alloc(bytes(0x800))
            parser = self.alloc(bytes(0xA00))
            u.mem_write(obj + 0x620, struct.pack("<Q", parser))
            u.mem_write(parser + 0x970, struct.pack("<I", 2))
            self.vec(obj + 0x628, [])
            u.reg_write(UC_ARM64_REG_X0, obj)
            u.reg_write(UC_ARM64_REG_X1, data)
            u.reg_write(UC_ARM64_REG_X2, self.alloc(struct.pack("<4d", *raw)))
            address = 0x9FDDC if self.device == "marble" else 0x95984
            if self.device == "mayfly":
                u.mem_write(obj + 0x620, bytes(24))
                u.mem_write(obj + 0x5D0, struct.pack("<I", int(dc)))
        u.emu_start(address, 0x20000000, count=10000000)
        assert u.reg_read(UC_ARM64_REG_PC) == 0x20000000, (
            "Stock computation did not return"
        )
        return (
            self.doubles(out, 4)
            if self.device == "mayfly" and not full
            else (self.bleed, self.lux)
        )
