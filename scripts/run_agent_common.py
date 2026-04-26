#!/usr/bin/env python3
"""Shared agent loop harness with selectable CLI backends."""

from __future__ import annotations

import argparse
import datetime as dt
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
LOG_DIR = REPO_ROOT / "build" / "agent_state" / "agent_logs"
LIMIT_PATTERNS = re.compile(r"hit your limit|rate limit|usage limit|quota", re.IGNORECASE)
LIMIT_HINT_PATTERNS = re.compile(r"hit your limit|rate limit|usage limit|quota|resets?", re.IGNORECASE)
WAIT_FOR_NEW_IDEA_PATTERN = re.compile(r"^WAIT_FOR_NEW_IDEA$")
IDLE_SLEEP_SECONDS = 3 * 3600  # 3 hours
WAIT_FOR_NEW_IDEA_TAIL_LINES = 12


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run an agent loop against the given prompt file.",
    )
    parser.add_argument(
        "prompt_file",
        nargs="?",
        default="AGENTS.md",
        help="Prompt markdown file to feed into the agent CLI. Defaults to AGENTS.md.",
    )
    parser.add_argument(
        "--cli",
        choices=("claude", "codex", "auto"),
        default=os.environ.get("AGENT_CLI", "codex"),
        help="Agent CLI backend to use. Defaults to AGENT_CLI or 'claude'.",
    )
    parser.add_argument(
        "--model",
        default=os.environ.get("AGENT_MODEL"),
        help="Optional model override for the selected CLI.",
    )
    parser.add_argument(
        "--codex-profile",
        default=os.environ.get("CODEX_PROFILE"),
        help="Optional Codex profile name to pass through.",
    )
    parser.add_argument(
        "--limit-resume-buffer-seconds",
        type=int,
        default=int(os.environ.get("LIMIT_RESUME_BUFFER_SECONDS", "600")),
        help="Extra wait time added after a reported rate-limit reset time.",
    )
    return parser.parse_args()


def ensure_log_dir() -> None:
    LOG_DIR.mkdir(parents=True, exist_ok=True)


def ensure_parent_dir(path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)


def resolve_prompt_file(prompt_file_arg: str) -> Path:
    prompt_path = Path(prompt_file_arg)
    if not prompt_path.is_absolute():
        prompt_path = REPO_ROOT / prompt_path
    prompt_path = prompt_path.resolve()
    if not prompt_path.is_file():
        raise FileNotFoundError(f"Prompt file not found: {prompt_file_arg}")
    return prompt_path


def available_cli(name: str) -> bool:
    return shutil.which(name) is not None


def resolve_cli(cli_arg: str) -> str:
    if cli_arg != "auto":
        if not available_cli(cli_arg):
            raise FileNotFoundError(f"Requested CLI '{cli_arg}' is not available on PATH.")
        return cli_arg

    for candidate in ("codex", "claude"):
        if available_cli(candidate):
            return candidate
    raise FileNotFoundError("Neither 'codex' nor 'claude' is available on PATH.")


def git_output(*args: str) -> str | None:
    try:
        result = subprocess.run(
            ["git", *args],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
        )
    except (FileNotFoundError, subprocess.CalledProcessError):
        return None
    return result.stdout.strip()


def current_commit() -> str:
    return git_output("rev-parse", "--short=6", "HEAD") or "no-git"


def worktree_is_clean() -> bool | None:
    status = git_output("status", "--short")
    if status is None:
        return None
    return status == ""


def stash_worktree_on_limit(mode: str, iteration: int, timestamp: str) -> None:
    inside_worktree = git_output("rev-parse", "--is-inside-work-tree")
    if inside_worktree != "true":
        print("[harness] Not a git worktree. Skip auto-stash.")
        return

    clean = worktree_is_clean()
    if clean is None:
        print("[harness] Unable to inspect worktree state. Skip auto-stash.")
        return
    if clean:
        print("[harness] Worktree clean. No stash needed.")
        return

    stash_message = f"auto-stash on limit: {mode} iter={iteration} at {timestamp}"
    result = subprocess.run(
        ["git", "stash", "push", "-u", "-m", stash_message],
        cwd=REPO_ROOT,
        capture_output=True,
        text=True,
    )
    if result.returncode == 0:
        print(f"[harness] Stashed in-progress work: {stash_message}")
    else:
        print("[harness] Failed to stash in-progress work before sleeping.")


def compute_wait_seconds_from_limit_line(limit_line: str) -> int:
    time_match = re.search(r"resets?\s+(\d{1,2})(?::(\d{2}))?\s*([ap]m)?", limit_line, re.IGNORECASE)
    tz_match = re.search(r"\(([^)]+)\)", limit_line)
    if not time_match:
        return 900

    hour = int(time_match.group(1))
    minute = int(time_match.group(2) or "0")
    ampm = (time_match.group(3) or "").lower()
    if ampm:
        if hour == 12:
            hour = 0
        if ampm == "pm":
            hour += 12

    tzinfo = None
    if tz_match:
        try:
            from zoneinfo import ZoneInfo

            tzinfo = ZoneInfo(tz_match.group(1).strip())
        except Exception:
            tzinfo = None

    now = dt.datetime.now(tz=tzinfo)
    target = now.replace(hour=hour, minute=minute, second=0, microsecond=0)
    if target <= now:
        target += dt.timedelta(days=1)
    return int((target - now).total_seconds())


def format_resume_time(wait_seconds: int) -> str:
    return (dt.datetime.now() + dt.timedelta(seconds=wait_seconds)).strftime("%Y-%m-%d %H:%M:%S")


def build_command(cli: str, prompt: str, args: argparse.Namespace) -> list[str]:
    if cli == "claude":
        command = [
            "claude",
            "--dangerously-skip-permissions",
            "--verbose",
            "--output-format",
            "stream-json",
            "--include-partial-messages",
            "-p",
            prompt,
            "--model",
            args.model or os.environ.get("CLAUDE_MODEL", "opus"),
        ]
        return command

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
        model = args.model or os.environ.get("CODEX_MODEL")
        if model:
            command.extend(["-m", model])
        command.append(prompt)
        return command

    raise ValueError(f"Unsupported CLI: {cli}")


def render_claude_event(raw_line: str) -> str:
    """Convert one stream-json line into a human-readable chunk.

    Returns "" to swallow the event. Output may be a partial chunk (no
    trailing newline) so streaming text/thinking deltas concatenate naturally.
    """
    raw_line = raw_line.strip()
    if not raw_line:
        return ""
    try:
        event = json.loads(raw_line)
    except json.JSONDecodeError:
        return raw_line + "\n"

    t = event.get("type")
    if t == "system":
        sub = event.get("subtype")
        if sub == "init":
            sid = (event.get("session_id") or "")[:8]
            return f"[claude] session {sid} init\n"
        if sub == "status":
            return f"[claude] {event.get('status', '')}\n"
        return ""
    if t == "stream_event":
        ev = event.get("event", {})
        et = ev.get("type")
        if et == "content_block_start":
            cb = ev.get("content_block", {}) or {}
            cb_type = cb.get("type")
            if cb_type == "thinking":
                return "\n[thinking] "
            if cb_type == "text":
                return "\n[assistant] "
            if cb_type == "tool_use":
                return f"\n[tool: {cb.get('name', '?')}] "
            return ""
        if et == "content_block_delta":
            d = ev.get("delta", {}) or {}
            dt_ = d.get("type")
            if dt_ == "thinking_delta":
                return d.get("thinking", "")
            if dt_ == "text_delta":
                return d.get("text", "")
            if dt_ == "input_json_delta":
                return d.get("partial_json", "")
            return ""
        if et == "message_stop":
            return "\n"
        return ""
    if t == "user":
        msg = event.get("message", {}) or {}
        out = []
        for c in msg.get("content", []) or []:
            if not isinstance(c, dict) or c.get("type") != "tool_result":
                continue
            tr = c.get("content", "")
            if isinstance(tr, list):
                parts = []
                for item in tr:
                    if isinstance(item, dict) and item.get("type") == "text":
                        parts.append(item.get("text", ""))
                tr = "".join(parts)
            head = (tr or "").splitlines()[0] if tr else ""
            out.append(f"\n[tool result] {head[:160]}")
        return ("".join(out) + "\n") if out else ""
    if t == "rate_limit_event":
        info = event.get("rate_limit_info", {}) or {}
        return f"\n[rate limit] status={info.get('status', '')}\n"
    if t == "result":
        ok = not event.get("is_error", True)
        ms = event.get("duration_ms", 0)
        return f"\n[claude result] success={ok} duration={ms}ms\n"
    return ""


def stream_process(command: list[str], logfile: Path, tmp_log_path: Path, cli: str) -> int:
    ensure_parent_dir(logfile)
    ensure_parent_dir(tmp_log_path)
    with logfile.open("w", encoding="utf-8", errors="replace") as log_fp, tmp_log_path.open(
        "w", encoding="utf-8", errors="replace"
    ) as tmp_fp:
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
            chunk = render_claude_event(line) if cli == "claude" else line
            if not chunk:
                continue
            sys.stdout.write(chunk)
            sys.stdout.flush()
            log_fp.write(chunk)
            log_fp.flush()
            tmp_fp.write(chunk)
            tmp_fp.flush()
        return process.wait()


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def build_prompt(cli: str, prompt_path: Path) -> str:
    prompt_text = read_text(prompt_path)
    if cli == "codex" and prompt_path == REPO_ROOT / "AGENTS.md":
        return (
            "Follow the repository instructions already auto-loaded from the workspace "
            "root AGENTS.md. This run was started by scripts/run_agent_common.py in "
            "scripted agent mode, so operate autonomously and avoid unnecessary "
            "clarifying questions unless blocked."
        )
    return prompt_text


def show_current_score() -> None:
    score_path = REPO_ROOT / "build" / "agent_state" / "last_result.txt"
    if score_path.is_file():
        print(score_path.read_text(encoding="utf-8", errors="replace").rstrip())
    else:
        print("(no result yet)")
    print()


def detect_limit_line(log_path: Path) -> str | None:
    if not log_path.is_file():
        return None
    for line in reversed(log_path.read_text(encoding="utf-8", errors="replace").splitlines()):
        if LIMIT_HINT_PATTERNS.search(line):
            return line
    return None


def log_contains_limit(log_path: Path) -> bool:
    if not log_path.is_file():
        return False
    return LIMIT_PATTERNS.search(log_path.read_text(encoding="utf-8", errors="replace")) is not None


def log_contains_wait_for_new_idea(log_path: Path) -> bool:
    if not log_path.is_file():
        return False
    lines = log_path.read_text(encoding="utf-8", errors="replace").splitlines()
    non_empty_lines = [line.strip() for line in lines if line.strip()]
    tail = non_empty_lines[-WAIT_FOR_NEW_IDEA_TAIL_LINES:]
    return any(WAIT_FOR_NEW_IDEA_PATTERN.fullmatch(line) for line in tail)


def git_fetch_has_new_commits() -> bool:
    """Run git fetch and check if the remote has new commits ahead of HEAD."""
    fetch_result = git_output("fetch", "--quiet")
    if fetch_result is None:
        return False
    # Check if upstream branch has commits we don't have
    behind = git_output("rev-list", "--count", "HEAD..@{upstream}")
    if behind is None:
        return False
    return int(behind) > 0


def main() -> int:
    args = parse_args()
    try:
        prompt_path = resolve_prompt_file(args.prompt_file)
        cli = resolve_cli(args.cli)
    except FileNotFoundError as exc:
        print(f"[harness] {exc}", file=sys.stderr)
        return 1

    os.chdir(REPO_ROOT)
    ensure_log_dir()

    mode = prompt_path.stem.lower()
    prompt = build_prompt(cli, prompt_path)
    inlines_prompt_file = not (cli == "codex" and prompt_path == REPO_ROOT / "AGENTS.md")

    print("[harness] Starting agent loop. Stop with Ctrl+C.")
    print(f"[harness] Prompt -> {prompt_path.relative_to(REPO_ROOT)}")
    print(f"[harness] CLI -> {cli}")
    if inlines_prompt_file:
        print("[harness] Prompt injection -> inline file contents")
    else:
        print("[harness] Prompt injection -> reuse Codex auto-loaded AGENTS.md")
    if args.model:
        print(f"[harness] Model override -> {args.model}")
    print("[harness] Logs -> build/agent_state/agent_logs/")

    iteration = 0
    while True:
        iteration += 1
        ensure_log_dir()
        commit = current_commit()
        timestamp = dt.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        logfile = LOG_DIR / f"{mode}_{cli}_iter_{iteration}_{commit}.log"
        fd, tmp_log_name = tempfile.mkstemp(prefix=f"{cli}-harness.", suffix=".log", dir="/tmp")
        os.close(fd)
        tmp_log_path = Path(tmp_log_name)

        print(f"[harness] === Iteration {iteration} at {timestamp} (commit: {commit}) ===")
        print(f"[harness] Log: {logfile.relative_to(REPO_ROOT)}")
        print("[harness] Preserving supervisor-managed test_before.log/test_after.log.")

        command = build_command(cli, prompt, args)
        exit_code = stream_process(command, logfile, tmp_log_path, cli)

        print(f"[harness] === Iteration {iteration} done. Current score: ===")
        show_current_score()

        limit_source = logfile if logfile.is_file() else tmp_log_path
        if log_contains_limit(limit_source):
            limit_line = detect_limit_line(limit_source) or ""
            base_wait_seconds = compute_wait_seconds_from_limit_line(limit_line)
            wait_seconds = base_wait_seconds + args.limit_resume_buffer_seconds
            resume_at = format_resume_time(wait_seconds)

            print(f"[harness] Detected token/rate limit in {limit_source}.")
            if limit_line:
                print(f"[harness] Limit hint: {limit_line}")
            stash_worktree_on_limit(mode, iteration, timestamp)
            print(
                "[harness] Sleeping "
                f"{wait_seconds}s (base={base_wait_seconds}s + buffer={args.limit_resume_buffer_seconds}s), "
                f"resume at {resume_at}."
            )
            tmp_log_path.unlink(missing_ok=True)
            time.sleep(wait_seconds)
            continue

        if log_contains_wait_for_new_idea(limit_source):
            print("[harness] Agent reported WAIT_FOR_NEW_IDEA. Checking remote for new work...")
            if git_fetch_has_new_commits():
                print("[harness] Remote has new commits. Pulling and restarting immediately.")
                subprocess.run(["git", "pull", "--ff-only"], cwd=REPO_ROOT)
                tmp_log_path.unlink(missing_ok=True)
                time.sleep(5)
                continue
            resume_at = format_resume_time(IDLE_SLEEP_SECONDS)
            print(
                f"[harness] No new remote commits. Sleeping {IDLE_SLEEP_SECONDS}s (~3h), "
                f"resume at {resume_at}."
            )
            tmp_log_path.unlink(missing_ok=True)
            time.sleep(IDLE_SLEEP_SECONDS)
            continue

        if exit_code != 0:
            print(f"[harness] {cli} exited with code {exit_code}. Continuing loop.")

        tmp_log_path.unlink(missing_ok=True)
        time.sleep(5)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except KeyboardInterrupt:
        print("\n[harness] Stopped by user.")
        raise SystemExit(130)
