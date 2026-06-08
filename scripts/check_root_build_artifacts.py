#!/usr/bin/env python3
"""Reject accidental compiler/CMake artifacts in the repository root."""

from __future__ import annotations

from pathlib import Path
import sys


REPO_ROOT = Path(__file__).resolve().parent.parent

ROOT_ARTIFACT_FILES = {
    "CMakeCache.txt",
    "Makefile",
    "cmake_install.cmake",
}
ROOT_ARTIFACT_DIRS = {
    "CMakeFiles",
}
ROOT_ARTIFACT_SUFFIXES = {
    ".a",
    ".d",
    ".dll",
    ".dylib",
    ".exe",
    ".lib",
    ".o",
    ".obj",
    ".out",
    ".so",
}


def main() -> int:
    offenders: list[Path] = []
    for child in REPO_ROOT.iterdir():
        if child.name in ROOT_ARTIFACT_FILES:
            offenders.append(child)
        elif child.is_dir() and child.name in ROOT_ARTIFACT_DIRS:
            offenders.append(child)
        elif child.is_file() and child.suffix.lower() in ROOT_ARTIFACT_SUFFIXES:
            offenders.append(child)

    if not offenders:
        print("Root build artifact check passed.")
        return 0

    print("Root build artifact check failed:")
    for path in sorted(offenders):
        print(f"- {path.relative_to(REPO_ROOT).as_posix()}")
    print()
    print("Use the Linux CMake presets from builds/linux_cmake so outputs stay under:")
    print("- builds/linux_cmake/build/linux-cardputer-zero-debug")
    print("- builds/linux_cmake/build/cardputer-zero-release")
    print("Use apps/linux_cardputer_zero/tools/build_cardputer_zero_deb.sh for .deb builds;")
    print("it stages source under /tmp and writes packages to build/cardputer-zero-deb/.")
    return 1


if __name__ == "__main__":
    sys.exit(main())
