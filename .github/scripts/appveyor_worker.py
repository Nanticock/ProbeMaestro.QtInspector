#!/usr/bin/env python

import argparse
import os
import subprocess
import sys


TARGET_OS_ENV_VAR = "PM_CI_OS"
COMPILER_ENV_VAR = "PM_CI_COMPILER"
COMPILER_VERSION_ENV_VAR = "PM_CI_COMPILER_VERSION"
ARCHITECTURE_ENV_VAR = "PM_CI_ARCHITECTURE"
MSVC_GENERATORS = {
    "2015": "Visual Studio 14 2015",
    "2017": "Visual Studio 15 2017",
    "2019": "Visual Studio 16 2019",
    "2022": "Visual Studio 17 2022",
}
WINDOWS_MSVC_IMAGES = {
    "2015": "Visual Studio 2015",
    "2017": "Visual Studio 2017",
    "2019": "Visual Studio 2019",
    "2022": "Visual Studio 2022",
}
LINUX_GCC_IMAGES = {
    "7": "Ubuntu",
    "8": "Ubuntu",
    "9": "Ubuntu2004",
    "10": "Ubuntu2204",
    "11": "Ubuntu2204",
    "12": "Ubuntu2204",
    "13": "Ubuntu2204",
}
LINUX_CLANG_IMAGES = {
    "9": "Ubuntu",
    "10": "Ubuntu",
    "11": "Ubuntu",
    "12": "Ubuntu2004",
    "13": "Ubuntu2004",
    "14": "Ubuntu2204",
    "15": "Ubuntu2204",
    "16": "Ubuntu2204",
    "17": "Ubuntu2204",
    "18": "Ubuntu2204",
    "19": "Ubuntu2204",
    "20": "Ubuntu2204",
}
MACOS_GCC_IMAGES = {
    "10": "macos-monterey",
    "11": "macos-ventura",
    "12": "macos-sonoma",
    "13": "macos-sonoma",
    "14": "macos-sonoma",
    "15": "macos-sonoma",
}
MACOS_CLANG_IMAGES = {
    "13": "macos-monterey",
    "14": "macos-ventura",
    "15": "macos-sonoma",
}


class AppVeyorWorkerError(RuntimeError):
    pass


def build_parser():
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


def resolve_compiler_spec(args, allow_env):
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
    return {
        "os_name": os_name,
        "compiler": compiler,
        "version": version,
        "architecture": architecture,
    }


def normalize_required_option(value, option_name):
    normalized = normalize_token(value)
    if normalized:
        return normalized
    raise AppVeyorWorkerError("Missing required option %s." % option_name)


def normalize_token(value):
    return str(value or "").strip().lower()


def normalize_os_name(os_name):
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
    raise AppVeyorWorkerError(
        "Unsupported build OS '%s'. Supported values: windows, linux, macos." % os_name
    )


def resolve_appveyor_build_configuration(spec, generator_override):
    os_name = spec["os_name"]
    compiler = spec["compiler"]
    if os_name == "windows" and compiler == "msvc":
        build_config = resolve_msvc_appveyor_build_configuration(spec)
    elif os_name == "linux" and compiler == "gcc":
        build_config = resolve_linux_gcc_appveyor_build_configuration(spec)
    elif os_name == "linux" and compiler == "clang":
        build_config = resolve_linux_clang_appveyor_build_configuration(spec)
    elif os_name == "macos" and compiler == "gcc":
        build_config = resolve_macos_gcc_appveyor_build_configuration(spec)
    elif os_name == "macos" and compiler == "clang":
        build_config = resolve_macos_clang_appveyor_build_configuration(spec)
    else:
        raise AppVeyorWorkerError(
            "Unsupported AppVeyor compiler configuration '%s %s %s %s'."
            % (spec["os_name"], spec["compiler"], spec["version"], spec["architecture"])
        )

    if not generator_override:
        return build_config

    build_config["generator"] = generator_override
    return build_config


def resolve_msvc_appveyor_build_configuration(spec):
    generator_version = normalize_msvc_version(spec["version"])
    generator = MSVC_GENERATORS.get(generator_version)
    if generator is None:
        supported = ", ".join(sorted(MSVC_GENERATORS))
        raise AppVeyorWorkerError(
            "Unsupported MSVC version '%s'. Supported versions: %s." % (spec["version"], supported)
        )

    architecture = normalize_msvc_architecture(spec["architecture"])
    image = WINDOWS_MSVC_IMAGES[generator_version]
    return {
        "image": image,
        "generator": generator,
        "configure_args": ["-A", architecture],
        "environment_variables": None,
    }


def resolve_linux_gcc_appveyor_build_configuration(spec):
    image = resolve_image(spec["version"], LINUX_GCC_IMAGES, "GCC")
    architecture = normalize_unix_appveyor_architecture(spec["architecture"], spec["os_name"], spec["compiler"])
    if architecture != "x64":
        raise AppVeyorWorkerError("AppVeyor Linux GCC builds currently support x64 only.")
    return {
        "image": image,
        "generator": "",
        "configure_args": [],
        "environment_variables": {"CC": "gcc-%s" % spec["version"], "CXX": "g++-%s" % spec["version"]},
    }


def resolve_linux_clang_appveyor_build_configuration(spec):
    image = resolve_image(spec["version"], LINUX_CLANG_IMAGES, "Clang")
    architecture = normalize_unix_appveyor_architecture(spec["architecture"], spec["os_name"], spec["compiler"])
    if architecture != "x64":
        raise AppVeyorWorkerError("AppVeyor Linux Clang builds currently support x64 only.")
    return {
        "image": image,
        "generator": "",
        "configure_args": [],
        "environment_variables": {"CC": "clang-%s" % spec["version"], "CXX": "clang++-%s" % spec["version"]},
    }


def resolve_macos_gcc_appveyor_build_configuration(spec):
    image = resolve_image(spec["version"], MACOS_GCC_IMAGES, "macOS GCC")
    architecture = normalize_unix_appveyor_architecture(spec["architecture"], spec["os_name"], spec["compiler"])
    if architecture != "x64":
        raise AppVeyorWorkerError("AppVeyor macOS GCC builds currently support x64 only.")
    return {
        "image": image,
        "generator": "",
        "configure_args": [],
        "environment_variables": {"CC": "gcc-%s" % spec["version"], "CXX": "g++-%s" % spec["version"]},
    }


def resolve_macos_clang_appveyor_build_configuration(spec):
    image = resolve_image(spec["version"], MACOS_CLANG_IMAGES, "macOS Clang")
    architecture = normalize_unix_appveyor_architecture(spec["architecture"], spec["os_name"], spec["compiler"])
    if architecture != "x64":
        raise AppVeyorWorkerError("AppVeyor macOS Clang builds currently support x64 only.")
    return {
        "image": image,
        "generator": "",
        "configure_args": [],
        "environment_variables": {"CC": "clang", "CXX": "clang++"},
    }


def resolve_image(version, supported_images, label):
    image = supported_images.get(version)
    if image:
        return image
    supported = ", ".join(sorted(supported_images))
    raise AppVeyorWorkerError(
        "Unsupported %s version '%s'. Supported versions: %s." % (label, version, supported)
    )


def normalize_msvc_version(version):
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


def normalize_msvc_architecture(architecture):
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
    raise AppVeyorWorkerError(
        "Unsupported MSVC architecture '%s'. Supported architectures: x86, x64, arm64." % architecture
    )


def normalize_unix_appveyor_architecture(architecture, os_name, compiler):
    aliases = {
        "x64": "x64",
        "amd64": "x64",
    }
    normalized = aliases.get(architecture)
    if normalized:
        return normalized
    raise AppVeyorWorkerError(
        "Unsupported %s %s architecture '%s'. Supported architectures: x64."
        % (os_name, compiler, architecture)
    )


def run_local_cmake_build(args, build_config):
    environment = os.environ.copy()
    if build_config["environment_variables"]:
        environment.update(build_config["environment_variables"])

    configure_cmd = ["cmake", "-B", args.build_dir, "-S", args.source_dir]
    if build_config["generator"]:
        configure_cmd.extend(["-G", build_config["generator"]])
    configure_cmd.extend(build_config["configure_args"])
    configure_cmd.extend(args.configure_arg)

    build_cmd = ["cmake", "--build", args.build_dir, "--config", args.config]
    build_cmd.extend(args.build_arg)

    run_command(configure_cmd, environment)
    run_command(build_cmd, environment)


def run_command(command, environment):
    write_line("Running: %s" % format_command(command))
    try:
        subprocess.check_call(command, env=environment)
    except OSError:
        raise AppVeyorWorkerError("Command not found: %s" % command[0])
    except subprocess.CalledProcessError as exc:
        raise AppVeyorWorkerError(
            "Command failed with exit code %s: %s" % (exc.returncode, format_command(command))
        )


def format_command(command):
    return " ".join([quote_command_part(part) for part in command])


def quote_command_part(value):
    if value == "":
        return '""'
    escaped = value.replace('"', '\\"')
    if any([char.isspace() for char in value]) or '"' in value:
        return '"%s"' % escaped
    return escaped


def write_line(message):
    sys.stdout.write(message + "\n")


def main(argv=None):
    args = build_parser().parse_args(argv)

    try:
        spec = resolve_compiler_spec(args, allow_env=True)
        build_config = resolve_appveyor_build_configuration(spec, generator_override=args.generator)
        run_local_cmake_build(args, build_config)
        return 0
    except AppVeyorWorkerError as exc:
        write_line("::error::%s" % exc)
        return 1


if __name__ == "__main__":
    sys.exit(main())