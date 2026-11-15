#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
"""Compare the source evaluator with stock instructions on the host."""

import argparse
import copy
import json
import math
from pathlib import Path
import random
import subprocess
import sys
import tempfile

from stock_reference import Reference

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from extract_model import extract


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--dumps",
        required=True,
        type=Path,
        help="Directory containing marble and mayfly dumps",
    )
    args = parser.parse_args()
    source = Path(__file__).resolve().parents[1]
    root = source.parents[4]
    objdump = sorted(
        (root / "prebuilts/clang/host/linux-x86").glob("clang-*/bin/llvm-objdump")
    )[-1]
    jsoncpp = root / "external/jsoncpp"
    with tempfile.TemporaryDirectory(prefix="udals-test-") as temporary:
        work = Path(temporary)
        runner = work / "model_runner"
        subprocess.run(
            [
                "g++",
                "-std=c++20",
                "-O2",
                "-ffp-contract=off",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-I" + str(source),
                "-I" + str(jsoncpp / "include"),
                str(source / "Compensation.cpp"),
                str(source / "tests/model_runner.cpp"),
                *[
                    str(jsoncpp / "src/lib_json" / name)
                    for name in ("json_reader.cpp", "json_value.cpp", "json_writer.cpp")
                ],
                "-o",
                str(runner),
            ],
            check=True,
        )
        for device in ("marble", "mayfly"):
            stock = args.dumps / device
            config = stock / "vendor/etc/sensors/config/lightSensorConfig.json"
            library = (
                stock
                / "vendor/lib64/hw/vendor.xiaomi.sensor.citsensorservice@2.0-impl.so"
            )
            model = extract(library, device)
            model_path = work / (device + ".json")
            model_path.write_text(json.dumps(model))
            random_source = random.Random(71)
            cases = []
            for brightness in (
                1,
                10,
                49,
                50,
                399,
                400,
                401,
                899,
                900,
                901,
                1700,
                1701,
                2047,
            ):
                for dc in (False, True):
                    rgb = [random_source.uniform(0, 8e7) for _ in range(3)]
                    raw = [random_source.uniform(0, 1e5) for _ in range(4)]
                    cases.append((brightness, dc, rgb, raw))
            # Exercise black, saturated primaries, mixed content and sensor saturation.
            for rgb in (
                (0, 0, 0),
                (8e7, 0, 0),
                (0, 8e7, 0),
                (0, 0, 8e7),
                (8e7, 8e7, 8e7),
            ):
                cases.append((2047, False, rgb, (100000, 10000, 10000, 10000)))
            cases.append(
                (2047, False, (0, 0, 0), (17000000, 17000000, 17000000, 17000000))
            )
            data = "".join(
                " ".join(map(str, [b, int(dc), *rgb, *raw])) + "\n"
                for b, dc, rgb, raw in cases
            )
            for calibrated in (False, True):
                ratio = (1.2, 0.8, 1.1, 0.9) if calibrated else (1, 1, 1, 1)
                scale = (0.9, 1.1, 0.8, 1.2) if calibrated else (1, 1, 1, 1)
                command = [str(runner), str(model_path), str(config)]
                if calibrated:
                    als = json.loads(config.read_text())["Als"]
                    calibration = {
                        "Als": {
                            "channel_scale": {"scale_data": ",".join(map(str, scale))}
                        }
                    }
                    for suffix in ("", "_dc"):
                        table = copy.deepcopy(als["panel_Info_cali_ori" + suffix])
                        table["data_w"] = []
                        for row in als["panel_Info_cali_ori" + suffix]["data_w"]:
                            values = list(map(float, row.split(",")))
                            table["data_w"].append(
                                ",".join(
                                    map(
                                        str,
                                        [
                                            values[0],
                                            *[v * r for v, r in zip(values[1:], ratio)],
                                        ],
                                    )
                                )
                            )
                        calibration["Als"]["panel_Info_cali" + suffix] = table
                    calibration_path = work / (device + "-calibration.json")
                    calibration_path.write_text(json.dumps(calibration))
                    command.append(str(calibration_path))
                output = subprocess.run(
                    command, input=data, text=True, capture_output=True, check=True
                ).stdout.splitlines()
                assert len(output) == len(cases)
                reference = Reference(device, args.dumps, objdump, scale, ratio)
                for case, line in zip(cases, output):
                    brightness, dc, rgb, raw = case
                    bleed, lux = reference.run(
                        (brightness, *rgb), raw=raw, full=True, dc=dc
                    )
                    expected = [*bleed, lux]
                    actual = list(map(float, line.split()))
                    assert all(
                        math.isclose(a, e, rel_tol=1e-10, abs_tol=1e-8)
                        for a, e in zip(actual, expected)
                    ), (device, calibrated, case, actual, expected)
            # No unsupported blob may accidentally generate a plausible-looking model.
            wrong = work / "wrong.so"
            wrong.write_bytes(library.read_bytes() + b"modified")
            try:
                extract(wrong, device)
            except ValueError:
                pass
            else:
                raise AssertionError("Modified stock ELF accepted")
            # Reject incomplete profiles instead of constructing a partial pipeline.
            del model["networks"]
            model_path.write_text(json.dumps(model))
            result = subprocess.run(
                [str(runner), str(model_path), str(config)], capture_output=True
            )
            assert result.returncode == 1
            print(
                f"{device}: {len(cases) * 2} stock comparisons, timeline/stride and rejection checks passed"
            )


if __name__ == "__main__":
    main()
