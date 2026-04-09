#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import time
from dataclasses import dataclass
from typing import Any, Iterable, Sequence
from urllib import error, request


DEFAULT_BASE_URL = "https://ci.appveyor.com"
SUCCESS_STATUSES = {"success"}
FAILURE_STATUSES = {"failed", "cancelled"}
APPVEYOR_BUILD_WORKER_IMAGE_ENV_VAR = "APPVEYOR_BUILD_WORKER_IMAGE"
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


class AppVeyorError(RuntimeError):
    pass


@dataclass(frozen=True)
class ProjectRef:
    account_name: str
    project_slug: str


@dataclass(frozen=True)
class BuildScript:
    language: str
    script: str

    def as_payload(self) -> dict[str, str]:
        return {"language": self.language, "script": self.script}


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


class AppVeyorClient:
    def __init__(self, token: str, account_name: str, base_url: str = DEFAULT_BASE_URL) -> None:
        self._token = token
        self._account_name = account_name
        self._base_url = base_url.rstrip("/")

    def get_project_settings(self, project: ProjectRef) -> dict[str, Any]:
        return self._request("GET", f"/api/projects/{project.account_name}/{project.project_slug}/settings")

    def update_project(self, settings: dict[str, Any]) -> None:
        self._request(
            "PUT",
            f"/api/account/{self._account_name}/projects",
            payload=settings,
            expected_statuses=(204,),
            response_type="none",
        )

    def set_project_build_scripts(
        self,
        project: ProjectRef,
        scripts: Sequence[BuildScript],
        build_mode: str = "script",
    ) -> str:
        response = self.get_project_settings(project)
        settings = response["settings"]
        configuration = settings.setdefault("configuration", {})
        previous_mode = str(configuration.get("buildMode", ""))
        configuration["buildMode"] = build_mode
        configuration["buildScripts"] = [script.as_payload() for script in scripts]
        self.update_project(settings)
        return previous_mode

    def start_build(
        self,
        project: ProjectRef,
        *,
        branch: str | None = None,
        commit_id: str | None = None,
        pull_request_id: str | None = None,
        environment_variables: dict[str, str] | None = None,
    ) -> dict[str, Any]:
        payload: dict[str, Any] = {
            "accountName": project.account_name,
            "projectSlug": project.project_slug,
        }

        if pull_request_id:
            payload["pullRequestId"] = pull_request_id
        else:
            if not branch:
                raise AppVeyorError("A branch is required when pull_request_id is not provided.")
            payload["branch"] = branch

        if commit_id:
            payload["commitId"] = commit_id

        if environment_variables:
            payload["environmentVariables"] = environment_variables

        return self._request(
            "POST",
            f"/api/account/{project.account_name}/builds",
            payload=payload,
        )

    def get_build(self, project: ProjectRef, build_version: str) -> dict[str, Any]:
        return self._request(
            "GET",
            f"/api/projects/{project.account_name}/{project.project_slug}/build/{build_version}",
        )

    def get_build_log(self, job_id: str) -> str:
        return self._request("GET", f"/api/buildjobs/{job_id}/log", response_type="text")

    def _request(
        self,
        method: str,
        path: str,
        *,
        payload: dict[str, Any] | None = None,
        expected_statuses: Sequence[int] = (200,),
        response_type: str = "json",
    ) -> Any:
        body: bytes | None = None
        headers = {
            "Authorization": f"Bearer {self._token}",
            "Accept": "application/json" if response_type == "json" else "text/plain",
        }

        if payload is not None:
            body = json.dumps(payload).encode("utf-8")
            headers["Content-Type"] = "application/json"

        req = request.Request(f"{self._base_url}{path}", data=body, headers=headers, method=method)

        try:
            with request.urlopen(req) as response:
                status = response.getcode()
                raw_body = response.read()
        except error.HTTPError as exc:
            error_body = exc.read().decode("utf-8", errors="replace").strip()
            detail = error_body or exc.reason
            raise AppVeyorError(f"{method} {path} failed with HTTP {exc.code}: {detail}") from exc
        except error.URLError as exc:
            raise AppVeyorError(f"{method} {path} failed: {exc.reason}") from exc

        if status not in expected_statuses:
            text = raw_body.decode("utf-8", errors="replace").strip()
            raise AppVeyorError(f"{method} {path} returned unexpected HTTP {status}: {text}")

        if response_type == "none":
            return None

        text = raw_body.decode("utf-8", errors="replace")
        if response_type == "text":
            return text
        if not text.strip():
            return {}

        try:
            return json.loads(text)
        except json.JSONDecodeError as exc:
            raise AppVeyorError(f"{method} {path} returned invalid JSON: {exc}") from exc


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Minimal AppVeyor CI helper for GitHub Actions workflows.",
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    common = argparse.ArgumentParser(add_help=False)
    common.add_argument("--account", help="AppVeyor account name. Defaults to APPVEYOR_ACCOUNT.")
    common.add_argument("--project", help="AppVeyor project slug. Defaults to APPVEYOR_PROJECT.")
    common.add_argument("--token", help="AppVeyor API token. Defaults to APPVEYOR_TOKEN.")
    common.add_argument(
        "--base-url",
        default=os.getenv("APPVEYOR_BASE_URL", DEFAULT_BASE_URL),
        help="AppVeyor base URL. Defaults to APPVEYOR_BASE_URL or https://ci.appveyor.com.",
    )

    cmake = argparse.ArgumentParser(add_help=False)
    cmake.add_argument("--source-dir", default=".", help="Source directory for cmake -S.")
    cmake.add_argument("--build-dir", default="build", help="Build directory for cmake -B.")
    cmake.add_argument("--config", default="Release", help="Build configuration passed to cmake --build.")
    cmake.add_argument(
        "--os",
        help="Build operating system such as windows, linux, or macos.",
    )
    cmake.add_argument(
        "--generator",
        help="Explicit CMake generator override. The target OS/compiler/version/architecture still determine the AppVeyor image.",
    )
    cmake.add_argument(
        "--compiler",
        help="Compiler family for the build target, such as msvc, gcc, or clang.",
    )
    cmake.add_argument(
        "--compiler-version",
        help="Compiler version for the build target, such as 2022, 2019, 15, or 14.",
    )
    cmake.add_argument(
        "--architecture",
        default="x64",
        help="Target architecture such as x86 or x64. Defaults to x64.",
    )
    cmake.add_argument(
        "--configure-arg",
        action="append",
        default=[],
        help="Extra argument appended to the cmake configure command. Can be repeated.",
    )
    cmake.add_argument(
        "--build-arg",
        action="append",
        default=[],
        help="Extra argument appended to the cmake build command. Can be repeated.",
    )

    configure_cmd = subparsers.add_parser(
        "configure-cmake-project",
        parents=[common, cmake],
        help="Update the AppVeyor project to build with cmake via PowerShell scripts.",
    )
    configure_cmd.set_defaults(handler=handle_configure_cmake_project)

    start_cmd = subparsers.add_parser(
        "start-build",
        parents=[common],
        help="Trigger an AppVeyor build.",
    )
    start_cmd.add_argument("--branch", help="Branch to build. Defaults to GitHub Actions branch variables.")
    start_cmd.add_argument("--commit-id", help="Specific commit to build.")
    start_cmd.add_argument("--pull-request-id", help="Pull request ID to build instead of a branch.")
    start_cmd.add_argument(
        "--env",
        action="append",
        default=[],
        metavar="NAME=VALUE",
        help="Build environment variable to send to AppVeyor. Can be repeated.",
    )
    start_cmd.set_defaults(handler=handle_start_build)

    wait_cmd = subparsers.add_parser(
        "wait-build",
        parents=[common],
        help="Poll AppVeyor until a build completes.",
    )
    wait_cmd.add_argument("--build-version", required=True, help="AppVeyor build version to watch.")
    wait_cmd.add_argument("--poll-interval", type=int, default=10, help="Polling interval in seconds.")
    wait_cmd.add_argument(
        "--timeout",
        type=int,
        default=0,
        help="Optional timeout in seconds. Zero waits indefinitely.",
    )
    wait_cmd.add_argument(
        "--log-lines",
        type=int,
        default=50,
        help="Number of trailing log lines to print on failure.",
    )
    wait_cmd.set_defaults(handler=handle_wait_build)

    run_cmd = subparsers.add_parser(
        "run-cmake-build",
        parents=[common, cmake],
        help="Configure the project for a cmake build, trigger it, and wait for completion.",
    )
    run_cmd.add_argument("--branch", help="Branch to build. Defaults to GitHub Actions branch variables.")
    run_cmd.add_argument("--commit-id", help="Specific commit to build.")
    run_cmd.add_argument("--pull-request-id", help="Pull request ID to build instead of a branch.")
    run_cmd.add_argument(
        "--env",
        action="append",
        default=[],
        metavar="NAME=VALUE",
        help="Build environment variable to send to AppVeyor. Can be repeated.",
    )
    run_cmd.add_argument("--poll-interval", type=int, default=10, help="Polling interval in seconds.")
    run_cmd.add_argument(
        "--timeout",
        type=int,
        default=0,
        help="Optional timeout in seconds. Zero waits indefinitely.",
    )
    run_cmd.add_argument(
        "--log-lines",
        type=int,
        default=50,
        help="Number of trailing log lines to print on failure.",
    )
    run_cmd.set_defaults(handler=handle_run_cmake_build)

    worker_cmd = subparsers.add_parser(
        "run-worker-cmake-build",
        parents=[cmake],
        help="Run the normalized CMake build on the current machine. Intended for AppVeyor build workers.",
    )
    worker_cmd.set_defaults(handler=handle_run_worker_cmake_build)

    return parser


def handle_configure_cmake_project(args: argparse.Namespace) -> int:
    project = resolve_project(args)
    client = build_client(args, project)
    scripts = create_cmake_worker_scripts(args)
    previous_mode = client.set_project_build_scripts(project, scripts)
    print(f"Configured AppVeyor project {project.project_slug} for cmake script mode.")
    if previous_mode:
        print(f"Previous buildMode: {previous_mode}")
    print(f"Worker script: {scripts[0].script}")
    return 0


def handle_start_build(args: argparse.Namespace) -> int:
    project = resolve_project(args)
    client = build_client(args, project)
    branch = None if args.pull_request_id else resolve_branch(args.branch)
    response = client.start_build(
        project,
        branch=branch,
        commit_id=args.commit_id,
        pull_request_id=args.pull_request_id,
        environment_variables=parse_environment_variables(args.env),
    )
    version = str(response.get("version", "")).strip()
    if not version:
        raise AppVeyorError(f"AppVeyor did not return a build version: {json.dumps(response, sort_keys=True)}")
    print(f"Started AppVeyor build version {version}.")
    print(f"BUILD_VERSION={version}")
    return 0


def handle_wait_build(args: argparse.Namespace) -> int:
    project = resolve_project(args)
    client = build_client(args, project)
    wait_for_build(
        client,
        project,
        args.build_version,
        poll_interval=max(args.poll_interval, 1),
        timeout=args.timeout,
        log_lines=max(args.log_lines, 1),
    )
    return 0


def handle_run_cmake_build(args: argparse.Namespace) -> int:
    project = resolve_project(args)
    client = build_client(args, project)
    spec = resolve_compiler_spec(args, allow_env=False)
    build_config = resolve_appveyor_build_configuration(spec, generator_override=args.generator)
    scripts = create_cmake_worker_scripts(args)
    previous_mode = client.set_project_build_scripts(project, scripts)
    print(f"Configured AppVeyor project {project.project_slug} for cmake script mode.")
    if previous_mode:
        print(f"Previous buildMode: {previous_mode}")

    branch = None if args.pull_request_id else resolve_branch(args.branch)
    response = client.start_build(
        project,
        branch=branch,
        commit_id=args.commit_id,
        pull_request_id=args.pull_request_id,
        environment_variables=build_request_environment_variables(
            spec,
            build_config,
            parse_environment_variables(args.env),
        ),
    )
    build_version = str(response.get("version", "")).strip()
    if not build_version:
        raise AppVeyorError(f"AppVeyor did not return a build version: {json.dumps(response, sort_keys=True)}")

    print(f"Started AppVeyor build version {build_version}.")
    wait_for_build(
        client,
        project,
        build_version,
        poll_interval=max(args.poll_interval, 1),
        timeout=args.timeout,
        log_lines=max(args.log_lines, 1),
    )
    return 0


def handle_run_worker_cmake_build(args: argparse.Namespace) -> int:
    spec = resolve_compiler_spec(args, allow_env=True)
    build_config = resolve_appveyor_build_configuration(spec, generator_override=args.generator)
    run_local_cmake_build(args, build_config)
    return 0


def build_client(args: argparse.Namespace, project: ProjectRef) -> AppVeyorClient:
    token = require_value(args.token or os.getenv("APPVEYOR_TOKEN"), "APPVEYOR_TOKEN", "AppVeyor token")
    return AppVeyorClient(token=token, account_name=project.account_name, base_url=args.base_url)


def resolve_project(args: argparse.Namespace) -> ProjectRef:
    account_name = require_value(args.account or os.getenv("APPVEYOR_ACCOUNT"), "APPVEYOR_ACCOUNT", "AppVeyor account")
    project_slug = require_value(args.project or os.getenv("APPVEYOR_PROJECT"), "APPVEYOR_PROJECT", "AppVeyor project")
    return ProjectRef(account_name=account_name, project_slug=project_slug)


def resolve_branch(explicit_branch: str | None) -> str:
    branch = explicit_branch or os.getenv("GITHUB_HEAD_REF") or os.getenv("GITHUB_REF_NAME")
    return require_value(branch, "GITHUB_HEAD_REF or GITHUB_REF_NAME", "build branch")


def require_value(value: str | None, env_name: str, label: str) -> str:
    if value:
        return value
    raise AppVeyorError(f"Missing {label}. Pass the CLI option or set {env_name}.")


def parse_environment_variables(items: Iterable[str]) -> dict[str, str] | None:
    environment: dict[str, str] = {}
    for item in items:
        name, separator, value = item.partition("=")
        if not separator or not name:
            raise AppVeyorError(f"Invalid environment variable '{item}'. Expected NAME=VALUE.")
        environment[name] = value
    return environment or None


def create_cmake_worker_scripts(args: argparse.Namespace) -> list[BuildScript]:
    worker_cmd = [
        "python",
        ".github/scripts/appveyor_cli.py",
        "run-worker-cmake-build",
        "--source-dir",
        args.source_dir,
        "--build-dir",
        args.build_dir,
        "--config",
        args.config,
    ]
    if args.generator:
        worker_cmd.extend(["--generator", args.generator])
    for configure_arg in args.configure_arg:
        worker_cmd.extend(["--configure-arg", configure_arg])
    for build_arg in args.build_arg:
        worker_cmd.extend(["--build-arg", build_arg])

    script_lines = [
        "$ErrorActionPreference = 'Stop'",
        join_powershell_command(worker_cmd),
        "if ($LastExitCode -ne 0) { exit $LastExitCode }",
    ]
    return [BuildScript(language="pwsh", script="\n".join(script_lines))]


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


def resolve_appveyor_build_configuration(
    spec: CompilerSpec,
    *,
    generator_override: str | None = None,
) -> AppVeyorBuildConfiguration:
    if spec.os_name == "windows" and spec.compiler == "msvc":
        build_config = resolve_msvc_appveyor_build_configuration(spec)
    elif spec.os_name == "linux" and spec.compiler == "gcc":
        build_config = resolve_linux_gcc_appveyor_build_configuration(spec)
    elif spec.os_name == "linux" and spec.compiler == "clang":
        build_config = resolve_linux_clang_appveyor_build_configuration(spec)
    elif spec.os_name == "macos" and spec.compiler == "gcc":
        build_config = resolve_macos_gcc_appveyor_build_configuration(spec)
    elif spec.os_name == "macos" and spec.compiler == "clang":
        build_config = resolve_macos_clang_appveyor_build_configuration(spec)
    else:
        raise AppVeyorError(
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


def normalize_required_option(value: str | None, option_name: str) -> str:
    normalized = normalize_token(value)
    if normalized:
        return normalized
    raise AppVeyorError(f"Missing required option {option_name}.")


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
    raise AppVeyorError(f"Unsupported build OS '{os_name}'. Supported values: windows, linux, macos.")


def resolve_msvc_appveyor_build_configuration(spec: CompilerSpec) -> AppVeyorBuildConfiguration:
    generator_version = normalize_msvc_version(spec.version)
    generator = MSVC_GENERATORS.get(generator_version)
    if generator is None:
        supported = ", ".join(sorted(MSVC_GENERATORS))
        raise AppVeyorError(f"Unsupported MSVC version '{spec.version}'. Supported versions: {supported}.")

    architecture = normalize_msvc_architecture(spec.architecture)
    configure_args = ("-A", architecture)
    image = WINDOWS_MSVC_IMAGES[generator_version]
    return AppVeyorBuildConfiguration(image=image, generator=generator, configure_args=configure_args)


def resolve_linux_gcc_appveyor_build_configuration(spec: CompilerSpec) -> AppVeyorBuildConfiguration:
    image = resolve_image(spec.version, LINUX_GCC_IMAGES, "GCC")
    architecture = normalize_unix_appveyor_architecture(spec.architecture, spec.os_name, spec.compiler)
    if architecture != "x64":
        raise AppVeyorError("AppVeyor Linux GCC builds currently support x64 only.")
    return AppVeyorBuildConfiguration(
        image=image,
        generator="",
        environment_variables={"CC": f"gcc-{spec.version}", "CXX": f"g++-{spec.version}"},
    )


def resolve_linux_clang_appveyor_build_configuration(spec: CompilerSpec) -> AppVeyorBuildConfiguration:
    image = resolve_image(spec.version, LINUX_CLANG_IMAGES, "Clang")
    architecture = normalize_unix_appveyor_architecture(spec.architecture, spec.os_name, spec.compiler)
    if architecture != "x64":
        raise AppVeyorError("AppVeyor Linux Clang builds currently support x64 only.")
    return AppVeyorBuildConfiguration(
        image=image,
        generator="",
        environment_variables={"CC": f"clang-{spec.version}", "CXX": f"clang++-{spec.version}"},
    )


def resolve_macos_gcc_appveyor_build_configuration(spec: CompilerSpec) -> AppVeyorBuildConfiguration:
    image = resolve_image(spec.version, MACOS_GCC_IMAGES, "macOS GCC")
    architecture = normalize_unix_appveyor_architecture(spec.architecture, spec.os_name, spec.compiler)
    if architecture != "x64":
        raise AppVeyorError("AppVeyor macOS GCC builds currently support x64 only.")
    return AppVeyorBuildConfiguration(
        image=image,
        generator="",
        environment_variables={"CC": f"gcc-{spec.version}", "CXX": f"g++-{spec.version}"},
    )


def resolve_macos_clang_appveyor_build_configuration(spec: CompilerSpec) -> AppVeyorBuildConfiguration:
    image = resolve_image(spec.version, MACOS_CLANG_IMAGES, "macOS Clang")
    architecture = normalize_unix_appveyor_architecture(spec.architecture, spec.os_name, spec.compiler)
    if architecture != "x64":
        raise AppVeyorError("AppVeyor macOS Clang builds currently support x64 only.")
    return AppVeyorBuildConfiguration(
        image=image,
        generator="",
        environment_variables={"CC": "clang", "CXX": "clang++"},
    )


def resolve_image(version: str, supported_images: dict[str, str], label: str) -> str:
    image = supported_images.get(version)
    if image:
        return image
    supported = ", ".join(sorted(supported_images))
    raise AppVeyorError(f"Unsupported {label} version '{version}'. Supported versions: {supported}.")


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
    raise AppVeyorError(
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
    raise AppVeyorError(
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
        raise AppVeyorError(f"Command not found: {command[0]}") from exc
    except subprocess.CalledProcessError as exc:
        raise AppVeyorError(f"Command failed with exit code {exc.returncode}: {format_command(command)}") from exc


def format_command(command: Sequence[str]) -> str:
    return " ".join(quote_powershell_argument(part) for part in command)


def join_powershell_command(parts: Sequence[str]) -> str:
    return " ".join(quote_powershell_argument(part) for part in parts)


def quote_powershell_argument(value: str) -> str:
    if value == "":
        return '""'
    escaped = value.replace("`", "``").replace('"', '`"')
    if any(char.isspace() for char in value) or any(char in value for char in '"`'):
        return f'"{escaped}"'
    return escaped


def wait_for_build(
    client: AppVeyorClient,
    project: ProjectRef,
    build_version: str,
    *,
    poll_interval: int,
    timeout: int,
    log_lines: int,
) -> None:
    start_time = time.monotonic()
    last_status: str | None = None

    while True:
        response = client.get_build(project, build_version)
        build = response.get("build", {})
        status = str(build.get("status", "unknown"))
        build_id = build.get("buildId")

        if status != last_status:
            print(f"Build {build_version} status: {status}")
            if build_id:
                print(f"Build URL: {build_url(project, build_id)}")
            last_status = status

        if status in SUCCESS_STATUSES:
            return

        if status in FAILURE_STATUSES:
            emit_build_logs(client, build, log_lines)
            raise AppVeyorError(f"AppVeyor build {build_version} finished with status {status}.")

        if timeout and time.monotonic() - start_time >= timeout:
            raise AppVeyorError(f"Timed out after {timeout} seconds waiting for AppVeyor build {build_version}.")

        time.sleep(poll_interval)


def emit_build_logs(client: AppVeyorClient, build: dict[str, Any], log_lines: int) -> None:
    jobs = build.get("jobs") or []
    selected_jobs = [job for job in jobs if job.get("status") in FAILURE_STATUSES] or jobs
    for job in selected_jobs:
        job_id = str(job.get("jobId", "")).strip()
        if not job_id:
            continue
        job_name = str(job.get("name") or "Build")
        print(f"Log tail for job '{job_name}' ({job_id}):")
        log_text = client.get_build_log(job_id)
        lines = log_text.splitlines()
        tail = lines[-log_lines:] if lines else []
        print("\n".join(tail) if tail else "<empty log>")


def build_url(project: ProjectRef, build_id: Any) -> str:
    return f"https://ci.appveyor.com/project/{project.account_name}/{project.project_slug}/builds/{build_id}"


def main(argv: Sequence[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    handler = getattr(args, "handler", None)
    if handler is None:
        parser.print_help()
        return 2

    try:
        return int(handler(args))
    except AppVeyorError as exc:
        print(f"::error::{exc}")
        return 1


if __name__ == "__main__":
    sys.exit(main())