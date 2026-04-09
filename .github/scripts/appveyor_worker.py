#!/usr/bin/env python

import argparse
import json
import os
import subprocess
import sys


TARGET_OS_ENV_VAR = "PM_CI_OS"
COMPILER_ENV_VAR = "PM_CI_COMPILER"
COMPILER_VERSION_ENV_VAR = "PM_CI_COMPILER_VERSION"
ARCHITECTURE_ENV_VAR = "PM_CI_ARCHITECTURE"
APPVEYOR_BUILD_WORKER_IMAGE_ENV_VAR = "APPVEYOR_BUILD_WORKER_IMAGE"
APPVEYOR_PROFILE_ID_ENV_VAR = "PM_CI_PROFILE_ID"
APPVEYOR_PROFILES_FILE = os.path.join(os.path.dirname(__file__), "appveyor_profiles.json")
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
    parser.add_argument("--profile", help="AppVeyor image profile id declared in .github/scripts/appveyor_profiles.json.")
    parser.add_argument("--image", help="Explicit AppVeyor build worker image such as Visual Studio 2017, Ubuntu2004, or macos-sonoma.")
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


def resolve_requested_appveyor_image(args, allow_env):
    if getattr(args, "image", None):
        return normalize_appveyor_image(args.image)
    if allow_env:
        return normalize_appveyor_image(os.getenv(APPVEYOR_BUILD_WORKER_IMAGE_ENV_VAR))
    return None


def resolve_requested_appveyor_profile(args, allow_env):
    if getattr(args, "profile", None):
        return normalize_profile_id(args.profile)
    if allow_env:
        return normalize_profile_id(os.getenv(APPVEYOR_PROFILE_ID_ENV_VAR))
    return None


def normalize_profile_id(value):
    normalized = str(value or "").strip()
    if normalized:
        return normalized
    return None


def load_appveyor_profiles():
    handle = open(APPVEYOR_PROFILES_FILE, "r")
    try:
        return json.load(handle)
    finally:
        handle.close()


def resolve_appveyor_profile(profile_id):
    payload = load_appveyor_profiles()
    for profile in payload.get("profiles", []):
        if str(profile.get("id", "")).strip() == profile_id:
            return profile
    supported = ", ".join(sorted([str(profile.get("id", "")).strip() for profile in payload.get("profiles", [])]))
    raise AppVeyorWorkerError("Unsupported AppVeyor profile '%s'. Supported profiles: %s." % (profile_id, supported))


def iter_profile_targets(profile):
    os_name = normalize_os_name(str(profile.get("os", "")))
    architectures = [normalize_required_option(value, "profile architectures") for value in profile.get("architectures", [])]
    compilers = profile.get("compilers", [])
    for compiler_group in compilers:
        compiler_name = normalize_required_option(compiler_group.get("name"), "profile compiler")
        versions = compiler_group.get("versions", [])
        for version in versions:
            normalized_version = normalize_required_option(version, "profile compiler version")
            for architecture in architectures:
                yield {
                    "os_name": os_name,
                    "compiler": compiler_name,
                    "version": normalized_version,
                    "architecture": architecture,
                }


def normalize_appveyor_image(image):
    value = str(image or "").strip()
    if value:
        return value
    return None


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


def resolve_appveyor_build_configuration(spec, image_override, generator_override):
    os_name = spec["os_name"]
    compiler = spec["compiler"]
    if os_name == "windows" and compiler == "msvc":
        build_config = resolve_msvc_appveyor_build_configuration(spec, image_override)
    elif os_name == "linux" and compiler == "gcc":
        build_config = resolve_linux_gcc_appveyor_build_configuration(spec, image_override)
    elif os_name == "linux" and compiler == "clang":
        build_config = resolve_linux_clang_appveyor_build_configuration(spec, image_override)
    elif os_name == "macos" and compiler == "gcc":
        build_config = resolve_macos_gcc_appveyor_build_configuration(spec, image_override)
    elif os_name == "macos" and compiler == "clang":
        build_config = resolve_macos_clang_appveyor_build_configuration(spec, image_override)
    else:
        raise AppVeyorWorkerError(
            "Unsupported AppVeyor compiler configuration '%s %s %s %s'."
            % (spec["os_name"], spec["compiler"], spec["version"], spec["architecture"])
        )

    if not generator_override:
        return build_config

    build_config["generator"] = generator_override
    return build_config


def resolve_msvc_appveyor_build_configuration(spec, image_override):
    generator_version = normalize_msvc_version(spec["version"])
    generator = MSVC_GENERATORS.get(generator_version)
    if generator is None:
        supported = ", ".join(sorted(MSVC_GENERATORS))
        raise AppVeyorWorkerError(
            "Unsupported MSVC version '%s'. Supported versions: %s." % (spec["version"], supported)
        )

    architecture = normalize_msvc_architecture(spec["architecture"])
    image = select_supported_image(
        generator_version,
        SUPPORTED_WINDOWS_MSVC_IMAGES,
        DEFAULT_WINDOWS_MSVC_IMAGE_BY_VERSION[generator_version],
        "MSVC",
        image_override,
    )
    return {
        "image": image,
        "generator": generator,
        "configure_args": ["-A", architecture],
        "environment_variables": None,
    }


def resolve_linux_gcc_appveyor_build_configuration(spec, image_override):
    image = select_supported_image(spec["version"], SUPPORTED_LINUX_GCC_IMAGES, "Ubuntu2004", "GCC", image_override)
    architecture = normalize_unix_appveyor_architecture(spec["architecture"], spec["os_name"], spec["compiler"])
    if architecture != "x64":
        raise AppVeyorWorkerError("AppVeyor Linux GCC builds currently support x64 only.")
    return {
        "image": image,
        "generator": "",
        "configure_args": [],
        "environment_variables": {"CC": "gcc-%s" % spec["version"], "CXX": "g++-%s" % spec["version"]},
    }


def resolve_linux_clang_appveyor_build_configuration(spec, image_override):
    image = select_supported_image(spec["version"], SUPPORTED_LINUX_CLANG_IMAGES, "Ubuntu2004", "Clang", image_override)
    architecture = normalize_unix_appveyor_architecture(spec["architecture"], spec["os_name"], spec["compiler"])
    if architecture != "x64":
        raise AppVeyorWorkerError("AppVeyor Linux Clang builds currently support x64 only.")
    return {
        "image": image,
        "generator": "",
        "configure_args": [],
        "environment_variables": {"CC": "clang-%s" % spec["version"], "CXX": "clang++-%s" % spec["version"]},
    }


def resolve_macos_gcc_appveyor_build_configuration(spec, image_override):
    image = select_supported_image(spec["version"], SUPPORTED_MACOS_GCC_IMAGES, "macos-sonoma", "macOS GCC", image_override)
    architecture = normalize_unix_appveyor_architecture(spec["architecture"], spec["os_name"], spec["compiler"])
    if architecture != "x64":
        raise AppVeyorWorkerError("AppVeyor macOS GCC builds currently support x64 only.")
    return {
        "image": image,
        "generator": "",
        "configure_args": [],
        "environment_variables": {"CC": "gcc-%s" % spec["version"], "CXX": "g++-%s" % spec["version"]},
    }


def resolve_macos_clang_appveyor_build_configuration(spec, image_override):
    image = select_supported_image(spec["version"], SUPPORTED_MACOS_CLANG_IMAGES, "macos-sonoma", "macOS Clang", image_override)
    architecture = normalize_unix_appveyor_architecture(spec["architecture"], spec["os_name"], spec["compiler"])
    if architecture != "x64":
        raise AppVeyorWorkerError("AppVeyor macOS Clang builds currently support x64 only.")
    return {
        "image": image,
        "generator": "",
        "configure_args": [],
        "environment_variables": {"CC": "clang", "CXX": "clang++"},
    }


def select_supported_image(version, supported_versions_by_image, default_image, label, image_override):
    if image_override:
        supported_versions = supported_versions_by_image.get(image_override)
        if supported_versions is None:
            supported_images = ", ".join(sorted(supported_versions_by_image))
            raise AppVeyorWorkerError(
                "Unsupported AppVeyor image '%s' for %s. Supported images: %s." % (image_override, label, supported_images)
            )
        if version not in supported_versions:
            raise AppVeyorWorkerError(
                "Unsupported %s version '%s' on AppVeyor image '%s'. Supported versions on that image: %s."
                % (label, version, image_override, ", ".join(supported_versions))
            )
        return image_override

    if version in supported_versions_by_image.get(default_image, ()):
        return default_image

    for image_name, versions in supported_versions_by_image.items():
        if version in versions:
            return image_name

    supported = sorted(set([supported_version for versions in supported_versions_by_image.values() for supported_version in versions]))
    raise AppVeyorWorkerError(
        "Unsupported %s version '%s'. Supported versions: %s." % (label, version, ", ".join(supported))
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


def run_local_cmake_build(args, spec, build_config):
    environment = os.environ.copy()
    if build_config["environment_variables"]:
        environment.update(build_config["environment_variables"])
    if spec["os_name"] == "macos" and spec["compiler"] == "clang":
        environment["DEVELOPER_DIR"] = select_macos_developer_dir(spec["version"])

    configure_cmd = ["cmake", "-B", args.build_dir, "-S", args.source_dir]
    if build_config["generator"]:
        configure_cmd.extend(["-G", build_config["generator"]])
    configure_cmd.extend(build_config["configure_args"])
    configure_cmd.extend(args.configure_arg)

    build_cmd = ["cmake", "--build", args.build_dir, "--config", args.config]
    build_cmd.extend(args.build_arg)

    run_command(configure_cmd, environment)
    run_command(build_cmd, environment)


def run_profile_builds(args, profile):
    failures = []
    targets = list(iter_profile_targets(profile))
    total_targets = len(targets)
    image_name = str(profile.get("image", "")).strip()

    if total_targets == 0:
        raise AppVeyorWorkerError("Profile '%s' does not define any targets." % str(profile.get("id", "<unknown>")))

    write_line(
        "Running AppVeyor profile %s on image %s with %s target(s)."
        % (str(profile.get("id", "<unknown>")), image_name, total_targets)
    )

    for index, spec in enumerate(targets, 1):
        label = build_target_label(spec)
        build_args = clone_args_for_target(args, label)
        write_line("[%s/%s] Starting %s" % (index, total_targets, label))
        try:
            build_config = resolve_appveyor_build_configuration(
                spec,
                image_override=image_name,
                generator_override=args.generator,
            )
            run_local_cmake_build(build_args, spec, build_config)
            write_line("[%s/%s] Succeeded %s" % (index, total_targets, label))
        except AppVeyorWorkerError as exc:
            failures.append((label, str(exc)))
            write_line("[%s/%s] Failed %s" % (index, total_targets, label))
            write_line("::error::%s" % exc)

    if failures:
        write_line("Profile summary: %s of %s target(s) failed." % (len(failures), total_targets))
        for label, message in failures:
            write_line("FAILED %s: %s" % (label, message))
        raise AppVeyorWorkerError("Profile '%s' failed for %s target(s)." % (str(profile.get("id", "<unknown>")), len(failures)))

    write_line("Profile summary: all %s target(s) succeeded." % total_targets)


def build_target_label(spec):
    return "%s-%s-%s" % (spec["compiler"], spec["version"], spec["architecture"])


def clone_args_for_target(args, label):
    return argparse.Namespace(
        source_dir=args.source_dir,
        build_dir=os.path.join(args.build_dir, label),
        config=args.config,
        os=args.os,
        profile=args.profile,
        image=args.image,
        generator=args.generator,
        compiler=args.compiler,
        compiler_version=args.compiler_version,
        architecture=args.architecture,
        configure_arg=list(args.configure_arg),
        build_arg=list(args.build_arg),
    )


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


def select_macos_developer_dir(version):
    requested_major = normalize_token(version)
    applications_dir = "/Applications"
    try:
        candidates = sorted(os.listdir(applications_dir))
    except OSError:
        raise AppVeyorWorkerError("Unable to inspect /Applications for installed Xcode versions.")

    for entry in candidates:
        if not entry.lower().startswith("xcode") or not entry.endswith(".app"):
            continue
        developer_dir = os.path.join(applications_dir, entry, "Contents", "Developer")
        xcodebuild_path = os.path.join(developer_dir, "usr", "bin", "xcodebuild")
        if not os.path.isfile(xcodebuild_path):
            continue
        try:
            output = subprocess.check_output([xcodebuild_path, "-version"])
        except (OSError, subprocess.CalledProcessError):
            continue
        version_text = output.decode("utf-8", "replace").splitlines()[0].strip()
        parts = version_text.split()
        if len(parts) < 2:
            continue
        installed_version = normalize_token(parts[1])
        if installed_version.split(".")[0] == requested_major:
            return developer_dir

    raise AppVeyorWorkerError(
        "No installed Xcode matched requested macOS clang version '%s' on image '%s'."
        % (version, os.getenv(APPVEYOR_BUILD_WORKER_IMAGE_ENV_VAR, "<unknown>"))
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
        profile_id = resolve_requested_appveyor_profile(args, allow_env=True)
        if profile_id:
            profile = resolve_appveyor_profile(profile_id)
            run_profile_builds(args, profile)
            return 0

        spec = resolve_compiler_spec(args, allow_env=True)
        build_config = resolve_appveyor_build_configuration(
            spec,
            image_override=resolve_requested_appveyor_image(args, allow_env=True),
            generator_override=args.generator,
        )
        run_local_cmake_build(args, spec, build_config)
        return 0
    except AppVeyorWorkerError as exc:
        write_line("::error::%s" % exc)
        return 1


if __name__ == "__main__":
    sys.exit(main())