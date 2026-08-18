#!/usr/bin/env python3
"""Fail if a macOS bundle's Intel slice requires newer than Catalina 10.15."""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

MAX_MINOS = (10, 15)


def parse_version(value: str) -> tuple[int, int, int]:
    parts = [int(p) for p in value.split(".") if p.isdigit()]
    while len(parts) < 3:
        parts.append(0)
    return parts[0], parts[1], parts[2]


def find_binary(bundle: Path) -> Path:
    macos_dir = bundle / "Contents" / "MacOS"
    if not macos_dir.is_dir():
        raise SystemExit(f"No Contents/MacOS directory in {bundle}")
    files = [path for path in macos_dir.iterdir() if path.is_file()]
    if not files:
        raise SystemExit(f"No executable in {macos_dir}")
    return files[0]


def extract_minos(otool_output: str) -> str:
    build = re.search(
        r"cmd LC_BUILD_VERSION\b(.*?)(?:\n {0,4}cmd |\Z)",
        otool_output,
        re.S,
    )
    if build:
        match = re.search(r"\bminos\s+([0-9]+(?:\.[0-9]+)*)", build.group(1))
        if match:
            return match.group(1)
    version_min = re.search(
        r"cmd LC_VERSION_MIN_MACOSX\b(.*?)(?:\n {0,4}cmd |\Z)",
        otool_output,
        re.S,
    )
    if version_min:
        match = re.search(r"\bversion\s+([0-9]+(?:\.[0-9]+)*)", version_min.group(1))
        if match:
            return match.group(1)
    match = re.search(r"\bminos\s+([0-9]+(?:\.[0-9]+)*)", otool_output)
    if match:
        return match.group(1)
    raise SystemExit("Could not find minos / LC_VERSION_MIN_MACOSX in otool output")


def inspect(bundle: Path) -> None:
    binary = find_binary(bundle)
    archs = subprocess.check_output(["lipo", "-archs", str(binary)], text=True).strip()
    print(f"{bundle}: archs={archs}")
    if "x86_64" not in archs.split():
        raise SystemExit(f"{bundle} is missing an x86_64 slice; Catalina 10.15 is Intel-only")
    otool = subprocess.check_output(
        ["otool", "-l", "-arch", "x86_64", str(binary)],
        text=True,
    )
    minos = extract_minos(otool)
    print(f"{bundle}: x86_64 minos={minos}")
    major, minor, _patch = parse_version(minos)
    if (major, minor) > MAX_MINOS:
        raise SystemExit(
            f"{bundle} x86_64 slice requires macOS {minos}; "
            "Catalina 10.15 needs minos 10.15 or older"
        )
    print(f"{bundle}: Catalina-compatible")


def main(argv: list[str]) -> int:
    if len(argv) < 2:
        print("usage: verify_macos_minos.py <bundle> [bundle...]", file=sys.stderr)
        return 2
    for argument in argv[1:]:
        inspect(Path(argument))
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
