#!/usr/bin/env python3

import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TESTS = [
    ROOT / "test" / "motion_alert_host_test.cpp",
]


def find_compiler() -> str:
    for name in ("c++", "g++", "clang++"):
        compiler = shutil.which(name)
        if compiler:
            return compiler
    raise RuntimeError("No C++ compiler found. Install c++, g++, or clang++ to run host tests.")


def main() -> int:
    compiler = find_compiler()
    with tempfile.TemporaryDirectory(prefix="xcam-host-tests-") as tmp:
        exe = Path(tmp) / ("motion_alert_host_test.exe" if os.name == "nt" else "motion_alert_host_test")
        cmd = [
            compiler,
            "-std=c++11",
            "-Wall",
            "-Wextra",
            "-Werror",
            f"-I{ROOT / 'include'}",
            str(TESTS[0]),
            "-o",
            str(exe),
        ]
        subprocess.check_call(cmd, cwd=str(ROOT))
        subprocess.check_call([str(exe)], cwd=str(ROOT))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)
