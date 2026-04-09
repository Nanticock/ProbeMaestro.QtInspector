#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import os
import sys
import time
from dataclasses import dataclass
from typing import Any, Iterable, Sequence
from urllib import error, request


DEFAULT_BASE_URL = "https://ci.appveyor.com"
SUCCESS_STATUSES = {"success"}
FAILURE_STATUSES = {"failed", "cancelled"}


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
    cmake.add_argument("--generator", required=True, help="CMake generator passed to cmake -G.")
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

    return parser


def handle_configure_cmake_project(args: argparse.Namespace) -> int:
    project = resolve_project(args)
    client = build_client(args, project)
    scripts = create_cmake_build_scripts(args)
    previous_mode = client.set_project_build_scripts(project, scripts)
    print(f"Configured AppVeyor project {project.project_slug} for cmake script mode.")
    if previous_mode:
        print(f"Previous buildMode: {previous_mode}")
    print(f"Configure script: {scripts[0].script}")
    print(f"Build script: {scripts[1].script}")
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
    scripts = create_cmake_build_scripts(args)
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
        environment_variables=parse_environment_variables(args.env),
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


def create_cmake_build_scripts(args: argparse.Namespace) -> list[BuildScript]:
    configure_cmd = join_powershell_command(
        [
            "cmake",
            "-B",
            args.build_dir,
            "-S",
            args.source_dir,
            "-G",
            args.generator,
            *args.configure_arg,
        ]
    )
    build_cmd = join_powershell_command(
        [
            "cmake",
            "--build",
            args.build_dir,
            "--config",
            args.config,
            *args.build_arg,
        ]
    )
    return [BuildScript(language="ps", script=configure_cmd), BuildScript(language="ps", script=build_cmd)]


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