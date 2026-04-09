#!/usr/bin/env python3

from __future__ import annotations

import argparse
import os
import subprocess
from dataclasses import dataclass
from typing import Sequence


APPVEYOR_BUILD_WORKER_IMAGE_ENV_VAR = "APPVEYOR_BUILD_WORKER_IMAGE"
TARGET_OS_ENV_VAR = "PM_CI_OS"
COMPILER_ENV_VAR = "PM_CI_COMPILER"
COMPILER_VERSION_ENV_VAR = "PM_CI_COMPILER_VERSION"
ARCHITECTURE_ENV_VAR = "PM_CI_ARCHITECTURE"
DEFAULT_WINDOWS_MSVC_IMAGE_BY_VERSION = {
    "2015": "Visual Studio 2015",
    "2017": "Visual Studio 2017",
    "2019": "Visual Studio 2019",
    "2022": "Visual Studio 2022",
}
SUPPORTED_WINDOWS_MSVC_IMAGES = {
    "Visual Studio 2015": ("2015",),
    "Visual Studio 2017": ("2015", "2017"),
    "Visual Studio 2019": ("2019",),
    "Visual Studio 2022": ("2022",),
}
SUPPORTED_LINUX_GCC_IMAGES = {
    "Ubuntu": ("7", "8", "9"),
    "Ubuntu2004": ("9", "10", "11"),
    "Ubuntu2204": ("9", "10", "11", "12", "13"),
}
SUPPORTED_LINUX_CLANG_IMAGES = {
    "Ubuntu": ("9", "10", "11", "12", "13", "14"),
    "Ubuntu2004": ("9", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20"),
    "Ubuntu2204": ("13", "14", "15", "16", "17", "18", "19", "20"),
}
SUPPORTED_MACOS_GCC_IMAGES = {
    "macos-monterey": ("10", "11", "12"),
    "macos-ventura": ("10", "11", "12"),
    "macos-sonoma": ("10", "11", "12"),
}
SUPPORTED_MACOS_CLANG_IMAGES = {
    "macos-monterey": ("13", "14"),
    "macos-ventura": ("13", "14", "15"),
    "macos-sonoma": ("13", "14", "15"),
}
MSVC_GENERATORS = {
    "2015": "Visual Studio 14 2015",
    "2017": "Visual Studio 15 2017",
    "2019": "Visual Studio 16 2019",
    "2022": "Visual Studio 17 2022",
}


class AppVeyorCMakeError(RuntimeError):
    pass


@dataclass(frozen=True)
class CompilerSpec:
    os_name: str
    compiler: str
    version: str
    architecture: str


@dataclass(frozen=True)
class AppVeyorBuildConfiguration:
    image: str
    generator: str
    configure_args: tuple[str, ...] = ()
    environment_variables: dict[str, str] | None = None


def build_request_environment_variables(
    spec: CompilerSpec,
    build_config: AppVeyorBuildConfiguration,
    extra_variables: dict[str, str] | None,
) -> dict[str, str]:
    environment = dict(extra_variables or {})
    environment[APPVEYOR_BUILD_WORKER_IMAGE_ENV_VAR] = build_config.image
    environment[TARGET_OS_ENV_VAR] = spec.os_name
    environment[COMPILER_ENV_VAR] = spec.compiler
    environment[COMPILER_VERSION_ENV_VAR] = spec.version
    environment[ARCHITECTURE_ENV_VAR] = spec.architecture
    return environment


def normalize_appveyor_image(image: str | None) -> str | None:
    value = str(image or "").strip()
    return value or None


def resolve_appveyor_build_configuration(
    spec: CompilerSpec,
    *,
    image_override: str | None = None,
    generator_override: str | None = None,
) -> AppVeyorBuildConfiguration:
    if spec.os_name == "windows" and spec.compiler == "msvc":
        build_config = resolve_msvc_appveyor_build_configuration(spec, image_override)
    elif spec.os_name == "linux" and spec.compiler == "gcc":
        build_config = resolve_linux_gcc_appveyor_build_configuration(spec, image_override)
    elif spec.os_name == "linux" and spec.compiler == "clang":
        build_config = resolve_linux_clang_appveyor_build_configuration(spec, image_override)
    elif spec.os_name == "macos" and spec.compiler == "gcc":
        build_config = resolve_macos_gcc_appveyor_build_configuration(spec, image_override)
    elif spec.os_name == "macos" and spec.compiler == "clang":
        build_config = resolve_macos_clang_appveyor_build_configuration(spec, image_override)
    else:
        raise AppVeyorCMakeError(
            "Unsupported AppVeyor compiler configuration "
            f"'{spec.os_name} {spec.compiler} {spec.version} {spec.architecture}'."
        )

    if not generator_override:
        return build_config

    return AppVeyorBuildConfiguration(
        image=build_config.image,
        generator=generator_override,
        configure_args=build_config.configure_args,
        environment_variables=build_config.environment_variables,
    )


def resolve_compiler_spec(args: argparse.Namespace, *, allow_env: bool) -> CompilerSpec:
    os_name = normalize_os_name(
        normalize_required_option(
            args.os if args.os else (os.getenv(TARGET_OS_ENV_VAR) if allow_env else None),
            "--os",
        )
    )
    compiler = normalize_required_option(
        args.compiler if args.compiler else (os.getenv(COMPILER_ENV_VAR) if allow_env else None),
        "--compiler",
    )
    version = normalize_required_option(
        args.compiler_version if args.compiler_version else (os.getenv(COMPILER_VERSION_ENV_VAR) if allow_env else None),
        "--compiler-version",
    )
    architecture = normalize_required_option(
        args.architecture if args.architecture else (os.getenv(ARCHITECTURE_ENV_VAR) if allow_env else None),
        "--architecture",
    )
    return CompilerSpec(os_name=os_name, compiler=compiler, version=version, architecture=architecture)


def resolve_requested_appveyor_image(args: argparse.Namespace, *, allow_env: bool) -> str | None:
    if getattr(args, "image", None):
        return normalize_appveyor_image(args.image)
    if allow_env:
        return normalize_appveyor_image(os.getenv(APPVEYOR_BUILD_WORKER_IMAGE_ENV_VAR))
    return None


def normalize_required_option(value: str | None, option_name: str) -> str:
    normalized = normalize_token(value)
    if normalized:
        return normalized
    raise AppVeyorCMakeError(f"Missing required option {option_name}.")


def normalize_token(value: str | None) -> str:
    return str(value or "").strip().lower()


def normalize_os_name(os_name: str) -> str:
    aliases = {
        "darwin": "macos",
        "linux": "linux",
        "mac": "macos",
        "macos": "macos",
        "osx": "macos",
        "ubuntu": "linux",
        "win": "windows",
        "windows": "windows",
    }
    normalized = aliases.get(os_name)
    if normalized:
        return normalized
    raise AppVeyorCMakeError(f"Unsupported build OS '{os_name}'. Supported values: windows, linux, macos.")


def resolve_msvc_appveyor_build_configuration(spec: CompilerSpec, image_override: str | None) -> AppVeyorBuildConfiguration:
    generator_version = normalize_msvc_version(spec.version)
    generator = MSVC_GENERATORS.get(generator_version)
    if generator is None:
        supported = ", ".join(sorted(MSVC_GENERATORS))
        raise AppVeyorCMakeError(f"Unsupported MSVC version '{spec.version}'. Supported versions: {supported}.")

    architecture = normalize_msvc_architecture(spec.architecture)
    configure_args = ("-A", architecture)
    image = select_supported_image(
        generator_version,
        SUPPORTED_WINDOWS_MSVC_IMAGES,
        DEFAULT_WINDOWS_MSVC_IMAGE_BY_VERSION[generator_version],
        "MSVC",
        image_override,
    )
    return AppVeyorBuildConfiguration(image=image, generator=generator, configure_args=configure_args)


def resolve_linux_gcc_appveyor_build_configuration(spec: CompilerSpec, image_override: str | None) -> AppVeyorBuildConfiguration:
    image = select_supported_image(spec.version, SUPPORTED_LINUX_GCC_IMAGES, "Ubuntu2004", "GCC", image_override)
    architecture = normalize_unix_appveyor_architecture(spec.architecture, spec.os_name, spec.compiler)
    if architecture != "x64":
        raise AppVeyorCMakeError("AppVeyor Linux GCC builds currently support x64 only.")
    return AppVeyorBuildConfiguration(
        image=image,
        generator="",
        environment_variables={"CC": f"gcc-{spec.version}", "CXX": f"g++-{spec.version}"},
    )


def resolve_linux_clang_appveyor_build_configuration(spec: CompilerSpec, image_override: str | None) -> AppVeyorBuildConfiguration:
    image = select_supported_image(spec.version, SUPPORTED_LINUX_CLANG_IMAGES, "Ubuntu2004", "Clang", image_override)
    architecture = normalize_unix_appveyor_architecture(spec.architecture, spec.os_name, spec.compiler)
    if architecture != "x64":
        raise AppVeyorCMakeError("AppVeyor Linux Clang builds currently support x64 only.")
    return AppVeyorBuildConfiguration(
        image=image,
        generator="",
        environment_variables={"CC": f"clang-{spec.version}", "CXX": f"clang++-{spec.version}"},
    )


def resolve_macos_gcc_appveyor_build_configuration(spec: CompilerSpec, image_override: str | None) -> AppVeyorBuildConfiguration:
    image = select_supported_image(spec.version, SUPPORTED_MACOS_GCC_IMAGES, "macos-sonoma", "macOS GCC", image_override)
    architecture = normalize_unix_appveyor_architecture(spec.architecture, spec.os_name, spec.compiler)
    if architecture != "x64":
        raise AppVeyorCMakeError("AppVeyor macOS GCC builds currently support x64 only.")
    return AppVeyorBuildConfiguration(
        image=image,
        generator="",
        environment_variables={"CC": f"gcc-{spec.version}", "CXX": f"g++-{spec.version}"},
    )


def resolve_macos_clang_appveyor_build_configuration(spec: CompilerSpec, image_override: str | None) -> AppVeyorBuildConfiguration:
    image = select_supported_image(spec.version, SUPPORTED_MACOS_CLANG_IMAGES, "macos-sonoma", "macOS Clang", image_override)
    architecture = normalize_unix_appveyor_architecture(spec.architecture, spec.os_name, spec.compiler)
    if architecture != "x64":
        raise AppVeyorCMakeError("AppVeyor macOS Clang builds currently support x64 only.")
    return AppVeyorBuildConfiguration(
        image=image,
        generator="",
        environment_variables={"CC": "clang", "CXX": "clang++"},
    )


def select_supported_image(
    version: str,
    supported_versions_by_image: dict[str, tuple[str, ...]],
    default_image: str,
    label: str,
    image_override: str | None,
) -> str:
    if image_override:
        supported_versions = supported_versions_by_image.get(image_override)
        if supported_versions is None:
            supported_images = ", ".join(sorted(supported_versions_by_image))
            raise AppVeyorCMakeError(f"Unsupported AppVeyor image '{image_override}' for {label}. Supported images: {supported_images}.")
        if version not in supported_versions:
            supported = ", ".join(supported_versions)
            raise AppVeyorCMakeError(
                f"Unsupported {label} version '{version}' on AppVeyor image '{image_override}'. Supported versions on that image: {supported}."
            )
        return image_override

    if version in supported_versions_by_image.get(default_image, ()):
        return default_image

    for image_name, versions in supported_versions_by_image.items():
        if version in versions:
            return image_name

    supported = sorted({supported_version for versions in supported_versions_by_image.values() for supported_version in versions})
    raise AppVeyorCMakeError(f"Unsupported {label} version '{version}'. Supported versions: {', '.join(supported)}.")


def normalize_msvc_version(version: str) -> str:
    aliases = {
        "14": "2015",
        "14.0": "2015",
        "15": "2017",
        "15.0": "2017",
        "16": "2019",
        "16.0": "2019",
        "17": "2022",
        "17.0": "2022",
    }
    return aliases.get(version, version)


def normalize_msvc_architecture(architecture: str) -> str:
    aliases = {
        "x86": "Win32",
        "win32": "Win32",
        "x64": "x64",
        "amd64": "x64",
        "arm64": "ARM64",
    }
    normalized = aliases.get(architecture)
    if normalized:
        return normalized
    raise AppVeyorCMakeError(
        f"Unsupported MSVC architecture '{architecture}'. Supported architectures: x86, x64, arm64."
    )


def normalize_unix_appveyor_architecture(architecture: str, os_name: str, compiler: str) -> str:
    aliases = {
        "x64": "x64",
        "amd64": "x64",
    }
    normalized = aliases.get(architecture)
    if normalized:
        return normalized
    raise AppVeyorCMakeError(
        f"Unsupported {os_name} {compiler} architecture '{architecture}'. Supported architectures: x64."
    )


def run_local_cmake_build(args: argparse.Namespace, build_config: AppVeyorBuildConfiguration) -> None:
    environment = os.environ.copy()
    if build_config.environment_variables:
        environment.update(build_config.environment_variables)

    configure_cmd = ["cmake", "-B", args.build_dir, "-S", args.source_dir]
    if build_config.generator:
        configure_cmd.extend(["-G", build_config.generator])
    configure_cmd.extend(build_config.configure_args)
    configure_cmd.extend(args.configure_arg)

    build_cmd = ["cmake", "--build", args.build_dir, "--config", args.config, *args.build_arg]

    run_command(configure_cmd, environment)
    run_command(build_cmd, environment)


def run_command(command: Sequence[str], environment: dict[str, str]) -> None:
    print(f"Running: {format_command(command)}")
    try:
        subprocess.run(command, env=environment, check=True)
    except FileNotFoundError as exc:
        raise AppVeyorCMakeError(f"Command not found: {command[0]}") from exc
    except subprocess.CalledProcessError as exc:
        raise AppVeyorCMakeError(f"Command failed with exit code {exc.returncode}: {format_command(command)}") from exc


def format_command(command: Sequence[str]) -> str:
    return " ".join(quote_command_part(part) for part in command)


def quote_command_part(value: str) -> str:
    if value == "":
        return '""'
    escaped = value.replace('"', '\\"')
    if any(char.isspace() for char in value) or '"' in value:
        return f'"{escaped}"'
    return escaped