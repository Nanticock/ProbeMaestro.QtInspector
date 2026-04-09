#!/usr/bin/env python3

from __future__ import annotations

import argparse
import sys
from typing import Sequence

from appveyor_cmake_support import AppVeyorCMakeError, resolve_appveyor_build_configuration, resolve_compiler_spec, run_local_cmake_build


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Run the normalized CMake build on the current AppVeyor worker.",
    )
    parser.add_argument("--source-dir", default=".", help="Source directory for cmake -S.")
    parser.add_argument("--build-dir", default="build", help="Build directory for cmake -B.")
    parser.add_argument("--config", default="Release", help="Build configuration passed to cmake --build.")
    parser.add_argument("--os", help="Build operating system such as windows, linux, or macos.")
    parser.add_argument(
        "--generator",
        help="Explicit CMake generator override. The target OS/compiler/version/architecture still determine the AppVeyor image.",
    )
    parser.add_argument("--compiler", help="Compiler family for the build target, such as msvc, gcc, or clang.")
    parser.add_argument(
        "--compiler-version",
        help="Compiler version for the build target, such as 2022, 2019, 15, or 14.",
    )
    parser.add_argument("--architecture", default="x64", help="Target architecture such as x86 or x64. Defaults to x64.")
    parser.add_argument(
        "--configure-arg",
        action="append",
        default=[],
        help="Extra argument appended to the cmake configure command. Can be repeated.",
    )
    parser.add_argument(
        "--build-arg",
        action="append",
        default=[],
        help="Extra argument appended to the cmake build command. Can be repeated.",
    )
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)

    try:
        spec = resolve_compiler_spec(args, allow_env=True)
        build_config = resolve_appveyor_build_configuration(spec, generator_override=args.generator)
        run_local_cmake_build(args, build_config)
        return 0
    except AppVeyorCMakeError as exc:
        print(f"::error::{exc}")
        return 1


if __name__ == "__main__":
    sys.exit(main())