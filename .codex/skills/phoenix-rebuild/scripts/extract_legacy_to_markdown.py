#!/usr/bin/env python3
"""Phoenix rebuild extractor harness.

Launches one agent process per matched legacy source file and asks it to
produce a compressed markdown companion beside the source file.
The extraction prompt is intentionally hardcoded here so stage-1 rebuild work
uses one stable contract.
"""

from __future__ import annotations

import argparse
import concurrent.futures
import os
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


SKILL_ROOT = Path(__file__).resolve().parent.parent
REPO_ROOT = SKILL_ROOT.parent.parent.parent
DEFAULT_LOG_DIR = REPO_ROOT / "build" / "agent_state" / "phoenix_rebuild"

PROMPT_TEMPLATE = """You are running the phoenix-rebuild stage-1 extraction workflow.

Task:
- Read the legacy source file at: {source_path}
- Write exactly one markdown extraction artifact at: {output_path}

Required output contract:
- Create or replace only the requested markdown file.
- Do not create any other files.
- Treat the source file as executable evidence, not as the new design.
- Produce a compressed extraction, not a source dump.
- Keep the markdown focused on:
  - purpose and current responsibility
  - important APIs and contract surfaces
  - dependency direction and hidden inputs
  - responsibility buckets
  - notable fast paths, compatibility paths, and overfit pressure
- Use short fenced `cpp` blocks only for essential surfaces.
- Do not copy long contiguous code from the source file.
- End with a short section describing what this file should and should not own in a rebuild.

Phoenix-specific rules:
- This repository uses exactly one non-helper header per directory as the only
  LLM index entry.
- `helper.hpp` is the only allowed exception and does not count as the
  directory index header.
- Do not invent a sibling header split for this file.
- If the source file is a non-helper `.hpp`, treat it as the directory index
  surface.
- Preserve relative naming by writing the artifact that corresponds to this exact source path.

Operate autonomously. Make the markdown artifact directly in the workspace.
"""


@dataclass(frozen=True)
class Job:
    source_path: Path
    output_path: Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run phoenix-rebuild extraction agents over globbed legacy source files.",
    )
    parser.add_argument(
        "patterns",
        nargs="+",
        help="Glob patterns relative to the repo root, for example src/backend/mir/x86/codegen/*.cpp",
    )
    parser.add_argument(
        "--cli",
        choices=("codex", "claude", "auto"),
        default=os.environ.get("AGENT_CLI", "codex"),
        help="Agent CLI backend.",
    )
    parser.add_argument(
        "--model",
        default=os.environ.get("AGENT_MODEL") or os.environ.get("CODEX_MODEL"),
        help="Optional model override.",
    )
    parser.add_argument(
        "--codex-profile",
        default=os.environ.get("CODEX_PROFILE"),
        help="Optional Codex profile override.",
    )
    parser.add_argument(
        "--jobs",
        type=int,
        default=max(1, min(8, (os.cpu_count() or 2))),
        help="Maximum concurrent extraction agents.",
    )
    parser.add_argument(
        "--log-dir",
        default=str(DEFAULT_LOG_DIR),
        help="Directory for per-file agent logs.",
    )
    return parser.parse_args()


def resolve_cli(cli_arg: str) -> str:
    if cli_arg == "auto":
        for candidate in ("codex", "claude"):
            if shutil.which(candidate):
                return candidate
        raise FileNotFoundError("Neither 'codex' nor 'claude' is available on PATH.")
    if not shutil.which(cli_arg):
        raise FileNotFoundError(f"Requested CLI '{cli_arg}' is not available on PATH.")
    return cli_arg


def expand_patterns(patterns: list[str]) -> list[Path]:
    matches: set[Path] = set()
    for pattern in patterns:
        for match in REPO_ROOT.glob(pattern):
            if match.is_file() and match.suffix in {".cpp", ".hpp"}:
                matches.add(match.resolve())
    return sorted(matches)


def is_helper_header(path: Path) -> bool:
    return path.name == "helper.hpp"


def validate_header_index_rule(paths: list[Path]) -> None:
    headers_by_parent: dict[Path, list[Path]] = {}
    for path in paths:
        if path.suffix != ".hpp" or is_helper_header(path):
            continue
        headers_by_parent.setdefault(path.parent.resolve(), []).append(path)
    collisions = {
        parent: sorted(items)
        for parent, items in headers_by_parent.items()
        if len(items) > 1
    }
    if not collisions:
        return
    lines = [
        "Phoenix rebuild aborted: multiple non-helper .hpp files found in one directory."
    ]
    for parent, items in sorted(collisions.items()):
        rendered = ", ".join(str(item.relative_to(REPO_ROOT)) for item in items)
        lines.append(f"- {parent.relative_to(REPO_ROOT)}: {rendered}")
    raise SystemExit("\n".join(lines))


def companion_markdown_path(source_path: Path) -> Path:
    return source_path.with_name(source_path.name + ".md")


def build_jobs(paths: list[Path]) -> list[Job]:
    jobs: list[Job] = []
    for path in paths:
        output_path = companion_markdown_path(path)
        jobs.append(Job(source_path=path, output_path=output_path))
    return jobs


def build_command(cli: str, prompt: str, args: argparse.Namespace) -> list[str]:
    if cli == "codex":
        command = [
            "codex",
            "exec",
            "--dangerously-bypass-approvals-and-sandbox",
            "--skip-git-repo-check",
            "-C",
            str(REPO_ROOT),
        ]
        if args.codex_profile:
            command.extend(["-p", args.codex_profile])
        if args.model:
            command.extend(["-m", args.model])
        command.append(prompt)
        return command
    if cli == "claude":
        command = [
            "claude",
            "--dangerously-skip-permissions",
            "-p",
            prompt,
        ]
        if args.model:
            command.extend(["--model", args.model])
        return command
    raise ValueError(f"Unsupported CLI: {cli}")


def slugify_source(source_path: Path) -> str:
    return str(source_path.relative_to(REPO_ROOT)).replace("/", "__")


def run_job(job: Job, cli: str, args: argparse.Namespace, log_dir: Path) -> tuple[Job, int, Path]:
    job.output_path.parent.mkdir(parents=True, exist_ok=True)
    log_dir.mkdir(parents=True, exist_ok=True)
    prompt = PROMPT_TEMPLATE.format(
        source_path=job.source_path,
        output_path=job.output_path,
    )
    command = build_command(cli, prompt, args)
    log_path = log_dir / f"{slugify_source(job.source_path)}.log"
    with log_path.open("w", encoding="utf-8", errors="replace") as log_fp:
        process = subprocess.Popen(
            command,
            cwd=REPO_ROOT,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
        )
        assert process.stdout is not None
        for line in process.stdout:
            sys.stdout.write(f"[{job.source_path.name}] {line}")
            sys.stdout.flush()
            log_fp.write(line)
        return job, process.wait(), log_path


def main() -> int:
    args = parse_args()
    cli = resolve_cli(args.cli)
    log_dir = Path(args.log_dir)
    if not log_dir.is_absolute():
        log_dir = (REPO_ROOT / log_dir).resolve()

    paths = expand_patterns(args.patterns)
    if not paths:
        print("No .cpp/.hpp files matched the requested glob patterns.", file=sys.stderr)
        return 1

    validate_header_index_rule(paths)
    jobs = build_jobs(paths)

    failures = 0
    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
        future_map = {
            pool.submit(run_job, job, cli, args, log_dir): job
            for job in jobs
        }
        for future in concurrent.futures.as_completed(future_map):
            job, returncode, log_path = future.result()
            status = "ok" if returncode == 0 else "failed"
            print(f"[phoenix] {status}: {job.source_path.relative_to(REPO_ROOT)} -> {job.output_path.relative_to(REPO_ROOT)}")
            if returncode != 0:
                failures += 1
                print(f"[phoenix] log: {log_path.relative_to(REPO_ROOT)}")

    if failures:
        print(f"[phoenix] {failures} extraction job(s) failed.", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
