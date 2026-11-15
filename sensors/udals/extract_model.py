#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
"""Extract UDALS model parameters, never executable code, from a supported stock ELF.

Addresses below are virtual addresses in the ELF (Ghidra's default image base
is subtracted by Elf.doubles). Unknown binaries are rejected: these parameters
are meaningful only together with the corresponding preprocessing algorithm.
"""

import argparse
import hashlib
import json
from pathlib import Path
import struct


class Elf:
    def __init__(self, path):
        self.data = Path(path).read_bytes()
        if self.data[:6] != b"\x7fELF\x02\x01":
            raise ValueError("Expected a little-endian ELF64 file")
        self.sha256 = hashlib.sha256(self.data).hexdigest()
        phoff = struct.unpack_from("<Q", self.data, 32)[0]
        entsize, count = struct.unpack_from("<HH", self.data, 54)
        self.segments = []
        for i in range(count):
            kind, _, offset, address, _, size, _, _ = struct.unpack_from(
                "<IIQQQQQQ", self.data, phoff + i * entsize
            )
            if kind == 1:
                self.segments.append((address, offset, size))

    def doubles(self, address, count=1):
        address -= 0x100000
        for base, offset, size in self.segments:
            if base <= address and address + count * 8 <= base + size:
                return list(
                    struct.unpack_from(f"<{count}d", self.data, offset + address - base)
                )
        raise ValueError(f"Unmapped model address {address:#x}")

    def scalar(self, address):
        return self.doubles(address)[0]


def network(elf, base, hidden, outputs, bias, exponent, divisor=None, negative="allow"):
    layers = []
    inputs = 4
    for width in (hidden, 64, 32, outputs):
        weights = elf.doubles(base, inputs * width)
        base += inputs * width * 8
        if width == outputs:
            biases = [elf.scalar(a) for a in bias]
        else:
            biases = elf.doubles(base, width)
            base += width * 8
        layers.append(
            {"inputs": inputs, "outputs": width, "weights": weights, "bias": biases}
        )
        inputs = width
    return {
        "layers": layers,
        "exponent": exponent,
        "divisor": divisor or [1.0] * outputs,
        "negative": negative,
    }


def extract(path, device):
    elf = Elf(path)
    if elf.sha256 != SUPPORTED[device]:
        raise ValueError(f"Unsupported {device} citsensor binary: {elf.sha256}")
    model = {
        "version": 1,
        "device": device,
        "source_sha256": elf.sha256,
        "block_size": 5,
        "networks": [],
    }
    if device == "marble":
        model.update(
            gamma=elf.scalar(0x112350),
            grid=19,
            spatial=[elf.doubles(a, 361) for a in (0x116C40, 0x117788, 0x1182D0)],
            input_offset=[10.0, 0.0, 0.0, 0.0],
            input_divisor=[
                2037.0,
                elf.scalar(0x112318),
                111953536.0,
                elf.scalar(0x1123B8),
            ],
        )
        for base, bias, exponent, divisor in (
            (0x118E48, 0x112338, elf.scalar(0x1123F0), 0x1123D8),
            (0x12E648, 0x1123C0, elf.scalar(0x112330), 0x112378),
            (0x143E48, 0x112410, 4.25, 0x1123C8),
            (0x159648, 0x112408, elf.scalar(0x112320), 0x1123E8),
        ):
            model["networks"].append(
                network(elf, base, 128, 1, [bias], [exponent], [elf.scalar(divisor)])
            )
    else:
        model.update(
            gamma=elf.scalar(0x10ED98),
            grid=25,
            spatial=[elf.doubles(0x113290, 625)] * 3,
        )
        for base, hidden, biases, exponents, negative in (
            (
                0x177F18,
                128,
                [0x10EE18, 0x10EE38, 0x10EDF8, 0x10ED90],
                [0x10EDC0, 0x10EDA8, 0x10EE10, 0x10EE50],
                "clamp",
            ),
            (
                0x162418,
                128,
                [0x10ED88, 0x10ED60, 0x10EDF0, 0x10ED70],
                [0x10EDC0, 0x10EDA8, 0x10EE10, 0x10EE30],
                "zero",
            ),
            (
                0x114618,
                256,
                [0x10EDD8, 0x10EE00, 0x10ED50, 0x10EDE0],
                [0x10EDE8, 0x10EDA0, 0x10EE58, 0x10ED68],
                "zero",
            ),
            (
                0x13B518,
                256,
                [0x10EE20, 0x10EE60, 0x10ED58, 0x10ED78],
                [None, 0x10EE48, 0x10EE08, 0x10ED80],
                "zero",
            ),
        ):
            model["networks"].append(
                network(
                    elf,
                    base,
                    hidden,
                    4,
                    biases,
                    [4.05 if a is None else elf.scalar(a) for a in exponents],
                    negative=negative,
                )
            )
        model["report_thresholds"] = [
            elf.scalar(a) for a in (0x10EE68, 0x10EDD0, 0x10EE28)
        ]
    return model


# Filled from the dumps used to recover the model layout; updating a blob requires
# validating its layout and numerical results, not just changing this digest.
SUPPORTED = {
    "marble": "186d5f610b9eb996242151b28f21c64d14b5452bd513fe57df019ae184a66577",
    "mayfly": "f2522ec06f1ca53d3b58db70d645dfddd27b1e226ad4e2f075ad720cf752e0af",
}


def fixup_model(ctx, file, file_path, device, **kwargs):
    model = extract(file_path, device)
    Path(file_path).write_text(json.dumps(model, separators=(",", ":")) + "\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("device", choices=("marble", "mayfly"))
    parser.add_argument("input")
    parser.add_argument("output")
    args = parser.parse_args()
    Path(args.output).write_text(
        json.dumps(extract(args.input, args.device), separators=(",", ":")) + "\n"
    )
