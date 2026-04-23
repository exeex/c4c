#!/usr/bin/env python3
import argparse
import json
import os
import re
import subprocess
import sys
from pathlib import Path

DEFAULT_TEST_BASELINE_LIMIT = 8
CTEST_SUMMARY_RE = re.compile(
    r"(^\d+% tests passed, .*?$[\s\S]*)", re.MULTILINE
)
SUMMARY_LINE_RE = re.compile(
    r"(?P<passed_pct>\d+)% tests passed, (?P<failed>\d+) tests failed out of (?P<total>\d+)"
)
FAILED_LIST_RE = re.compile(r"^\s*\d+\s*-\s*(?P<name>[A-Za-z0-9_.+-]+)\s+\(Failed\)\s+")


CURRENT_STEP_ID_RE = re.compile(r"^Current Step ID:\s*(.*?)\s*$", re.MULTILINE)
CURRENT_STEP_TITLE_RE = re.compile(r"^Current Step Title:\s*(.*?)\s*$", re.MULTILINE)
PLAN_REVIEW_COUNTER_RE = re.compile(
    r"^Plan Review Counter:\s*(\d+)\s*/\s*(\d+)\s*$", re.MULTILINE
)
CODE_REVIEW_REMINDER = "你該做code review了"
BASELINE_SANITY_REMINDER = "你該做baseline sanity check了"
CODE_REVIEW_REMINDER_RE = re.compile(
    rf"^{re.escape(CODE_REVIEW_REMINDER)}\s*$", re.MULTILINE
)
BASELINE_SANITY_REMINDER_RE = re.compile(
    rf"^{re.escape(BASELINE_SANITY_REMINDER)}\s*$", re.MULTILINE
)
LEGACY_CURRENT_STEP_RE = re.compile(r"^Step\s+([0-9.]+)\s+(.*?)\s*$")
LIFECYCLE_TAG_PREFIX_RE = re.compile(
    r"^\[(?:plan|idea|todo_only)(?:\+(?:plan|idea|todo_only))*\]\s+"
)


def repo_root() -> Path:
    return Path(__file__).resolve().parent.parent


def default_paths():
    root = repo_root()
    return root / "todo.md", root / ".plan_review_state.json", root / "test_baseline.log"


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def write_text(path: Path, text: str) -> None:
    path.write_text(text, encoding="utf-8")


def replace_file(src: Path, dst: Path) -> None:
    os.replace(src, dst)


def parse_todo(path: Path) -> dict:
    text = read_text(path)
    step_id_match = CURRENT_STEP_ID_RE.search(text)
    step_title_match = CURRENT_STEP_TITLE_RE.search(text)
    if not step_id_match:
        raise SystemExit(f"missing 'Current Step ID:' line in {path}")
    if not step_title_match:
        raise SystemExit(f"missing 'Current Step Title:' line in {path}")
    return {
        "text": text,
        "current_step_id": step_id_match.group(1),
        "current_step_title": step_title_match.group(1),
    }


def default_state() -> dict:
    return {
        "current_step_id": "none",
        "current_step_title": "none",
        "counter": 0,
        "review_limit": 5,
        "code_review_pending": False,
        "test_baseline_counter": 0,
        "test_baseline_limit": DEFAULT_TEST_BASELINE_LIMIT,
        "test_baseline_regex": "",
        "baseline_sanity_pending": False,
    }


def replace_once(text: str, pattern: re.Pattern, replacement: str) -> str:
    new_text, count = pattern.subn(replacement, text, count=1)
    if count != 1:
        raise SystemExit("expected exactly one metadata line to replace in todo.md")
    return new_text


def remove_optional_line(text: str, pattern: re.Pattern) -> str:
    return pattern.sub("", text)


def strip_reminder_lines(text: str) -> str:
    text = remove_optional_line(text, CODE_REVIEW_REMINDER_RE)
    text = remove_optional_line(text, BASELINE_SANITY_REMINDER_RE)
    text = remove_optional_line(text, PLAN_REVIEW_COUNTER_RE)
    text = re.sub(
        r"(Current Step Title:\s*.*)\n{3,}(# Current Packet)",
        r"\1\n\n\2",
        text,
        count=1,
    )
    return text


def sync_reminder_lines(text: str, state: dict) -> str:
    reminder_lines = []
    if state["code_review_pending"]:
        reminder_lines.append(CODE_REVIEW_REMINDER)
    if state["baseline_sanity_pending"]:
        reminder_lines.append(BASELINE_SANITY_REMINDER)

    if not reminder_lines:
        return text

    lines = text.splitlines()
    text_ends_with_newline = text.endswith("\n")

    for index, line in enumerate(lines):
        if CURRENT_STEP_TITLE_RE.match(line):
            insert_at = index + 1
            for offset, reminder in enumerate(reminder_lines):
                lines.insert(insert_at + offset, reminder)
            next_line_index = insert_at + len(reminder_lines)
            if next_line_index >= len(lines) or lines[next_line_index].strip():
                lines.insert(next_line_index, "")
            return "\n".join(lines) + ("\n" if text_ends_with_newline else "")

    raise SystemExit("missing 'Current Step Title:' line while syncing reminder lines")


def sync_todo(path: Path, state: dict) -> None:
    parsed = parse_todo(path)
    text = parsed["text"]
    text = replace_once(
        text,
        CURRENT_STEP_ID_RE,
        f"Current Step ID: {state['current_step_id']}",
    )
    text = replace_once(
        text,
        CURRENT_STEP_TITLE_RE,
        f"Current Step Title: {state['current_step_title']}",
    )
    text = strip_reminder_lines(text)
    text = sync_reminder_lines(text, state)
    if text != parsed["text"]:
        write_text(path, text)


def load_state(path: Path) -> dict | None:
    if not path.exists():
        return None
    with path.open("r", encoding="utf-8") as handle:
        data = json.load(handle)
    if "current_step_id" not in data or "current_step_title" not in data:
        legacy_step = data.get("current_step")
        if legacy_step is None:
            raise SystemExit(f"missing step metadata in {path}")
        match = LEGACY_CURRENT_STEP_RE.match(legacy_step)
        if match:
            data["current_step_id"] = match.group(1)
            data["current_step_title"] = match.group(2)
        else:
            data["current_step_id"] = legacy_step
            data["current_step_title"] = "none"
    for key in ("current_step_id", "current_step_title", "counter", "review_limit"):
        if key not in data:
            raise SystemExit(f"missing '{key}' in {path}")
    if "code_review_pending" not in data:
        data["code_review_pending"] = False
    if "test_baseline_counter" not in data:
        data["test_baseline_counter"] = 0
    if "test_baseline_limit" not in data:
        data["test_baseline_limit"] = DEFAULT_TEST_BASELINE_LIMIT
    if "test_baseline_regex" not in data:
        data["test_baseline_regex"] = ""
    if "baseline_sanity_pending" not in data:
        data["baseline_sanity_pending"] = False
    return data


def save_state(path: Path, state: dict) -> None:
    payload = {
        "current_step_id": state["current_step_id"],
        "current_step_title": state["current_step_title"],
        "counter": state["counter"],
        "review_limit": state["review_limit"],
        "code_review_pending": state["code_review_pending"],
        "test_baseline_counter": state["test_baseline_counter"],
        "test_baseline_limit": state["test_baseline_limit"],
        "test_baseline_regex": state["test_baseline_regex"],
        "baseline_sanity_pending": state["baseline_sanity_pending"],
    }
    write_text(path, json.dumps(payload, indent=2, sort_keys=True) + "\n")


def init_from_todo(todo_path: Path, state_path: Path) -> dict:
    parsed = parse_todo(todo_path)
    state = default_state()
    state["current_step_id"] = parsed["current_step_id"]
    state["current_step_title"] = parsed["current_step_title"]
    save_state(state_path, state)
    return state


def git(*args: str) -> str:
    completed = subprocess.run(
        ["git", *args],
        check=True,
        capture_output=True,
        text=True,
        cwd=repo_root(),
    )
    return completed.stdout


def current_head_hash() -> str:
    return git("rev-parse", "HEAD").strip()


def current_head_subject() -> str:
    return git("log", "-1", "--format=%s").strip()


def head_has_lifecycle_tag() -> bool:
    return bool(LIFECYCLE_TAG_PREFIX_RE.match(current_head_subject()))


def head_commit_paths() -> set[str]:
    output = git("diff-tree", "--no-commit-id", "--name-only", "-r", "--diff-filter=ACMR", "HEAD")
    return {line.strip() for line in output.splitlines() if line.strip()}


def ensure_state(todo_path: Path, state_path: Path) -> dict:
    state = load_state(state_path)
    if state is None:
        if todo_path.exists():
            state = init_from_todo(todo_path, state_path)
        else:
            state = default_state()
            save_state(state_path, state)
    return state


def capture_ctest_summary(text: str) -> str:
    match = CTEST_SUMMARY_RE.search(text)
    if match:
        return match.group(1).lstrip()
    stripped = text.strip()
    return stripped if stripped else "ctest produced no summary output\n"


def parse_baseline_summary(text: str) -> dict:
    summary_match = None
    failed_tests = set()
    for line in text.splitlines():
        if summary_match is None:
            summary_match = SUMMARY_LINE_RE.search(line) or summary_match
        failed_match = FAILED_LIST_RE.match(line)
        if failed_match:
            failed_tests.add(failed_match.group("name"))

    if summary_match is None:
        raise SystemExit("baseline summary is missing the ctest summary line")

    total = int(summary_match.group("total"))
    failed = int(summary_match.group("failed"))
    passed = total - failed
    return {
        "total": total,
        "failed": failed,
        "passed": passed,
        "failed_tests": failed_tests,
    }


def format_baseline_log(summary: str, baseline_regex: str) -> str:
    baseline_scope = baseline_regex if baseline_regex else "<full-suite>"
    header = (
        f"Baseline Commit: {current_head_hash()}\n"
        f"Baseline Subject: {current_head_subject()}\n\n"
        f"Baseline Regex: {baseline_scope}\n\n"
    )
    body = summary if summary.endswith("\n") else summary + "\n"
    return header + body


def baseline_has_not_regressed(previous_path: Path, candidate_path: Path) -> bool:
    previous = parse_baseline_summary(read_text(previous_path))
    candidate = parse_baseline_summary(read_text(candidate_path))
    if candidate["passed"] < previous["passed"]:
        return False
    if candidate["failed_tests"] - previous["failed_tests"]:
        return False
    return True


def run_command(command: list[str], *, check: bool) -> subprocess.CompletedProcess:
    return subprocess.run(
        command,
        cwd=repo_root(),
        capture_output=True,
        text=True,
        check=check,
    )


def refresh_test_baseline(baseline_path: Path, baseline_regex: str) -> None:
    root = repo_root()
    build_dir = root / "build"
    candidate_path = baseline_path.with_name("test_baseline.new.log")

    if not (build_dir / "CMakeCache.txt").exists():
        run_command(["cmake", "-S", str(root), "-B", str(build_dir)], check=True)

    run_command(["cmake", "--build", str(build_dir), "-j"], check=True)
    ctest_command = ["ctest", "--test-dir", str(build_dir), "-j", "--output-on-failure"]
    if baseline_regex:
        ctest_command.extend(["-R", baseline_regex])
    ctest = run_command(ctest_command, check=False)
    combined = ctest.stdout
    if ctest.stderr:
        combined = combined + ("\n" if combined and not combined.endswith("\n") else "") + ctest.stderr
    summary = capture_ctest_summary(combined)
    write_text(candidate_path, format_baseline_log(summary, baseline_regex))

    if not baseline_path.exists():
        replace_file(candidate_path, baseline_path)
        return

    if baseline_has_not_regressed(baseline_path, candidate_path):
        replace_file(candidate_path, baseline_path)
        return

    candidate_path.unlink(missing_ok=True)


def cmd_show(args) -> int:
    state = ensure_state(args.todo, args.state)
    print(json.dumps(state, indent=2, sort_keys=True))
    return 0


def cmd_reset(args) -> int:
    state = ensure_state(args.todo, args.state)
    if args.step_id is not None:
        state["current_step_id"] = args.step_id
    if args.step_title is not None:
        state["current_step_title"] = args.step_title
    state["counter"] = 0
    state["code_review_pending"] = False
    state["baseline_sanity_pending"] = False
    save_state(args.state, state)
    sync_todo(args.todo, state)
    return 0


def cmd_set_step(args) -> int:
    state = ensure_state(args.todo, args.state)
    changed = (
        state["current_step_id"] != args.step_id
        or state["current_step_title"] != args.step_title
    )
    if changed:
        state["current_step_id"] = args.step_id
        state["current_step_title"] = args.step_title
        state["counter"] = 0
        state["code_review_pending"] = False
        state["baseline_sanity_pending"] = False
    save_state(args.state, state)
    sync_todo(args.todo, state)
    return 0


def cmd_increment(args) -> int:
    state = ensure_state(args.todo, args.state)
    state["counter"] += 1
    if state["counter"] >= state["review_limit"]:
        state["code_review_pending"] = True
    save_state(args.state, state)
    sync_todo(args.todo, state)
    return 0


def cmd_set_limit(args) -> int:
    state = ensure_state(args.todo, args.state)
    state["review_limit"] = args.review_limit
    state["code_review_pending"] = state["counter"] >= state["review_limit"]
    save_state(args.state, state)
    sync_todo(args.todo, state)
    return 0


def cmd_set_baseline_limit(args) -> int:
    state = ensure_state(args.todo, args.state)
    state["test_baseline_limit"] = args.test_baseline_limit
    state["baseline_sanity_pending"] = (
        state["test_baseline_counter"] >= state["test_baseline_limit"]
    )
    save_state(args.state, state)
    if args.todo.exists():
        sync_todo(args.todo, state)
    return 0


def cmd_set_baseline_regex(args) -> int:
    state = ensure_state(args.todo, args.state)
    state["test_baseline_regex"] = args.test_baseline_regex
    save_state(args.state, state)
    if args.todo.exists():
        sync_todo(args.todo, state)
    return 0


def cmd_post_commit(args) -> int:
    state = ensure_state(args.todo, args.state)
    previous_code_review_pending = state["code_review_pending"]
    previous_baseline_sanity_pending = state["baseline_sanity_pending"]
    should_print_code_review = False
    should_print_baseline_sanity = False
    changed_paths = head_commit_paths()
    has_plan_change = "plan.md" in changed_paths
    has_todo_change = "todo.md" in changed_paths
    is_lifecycle_commit = head_has_lifecycle_tag()

    if args.todo.exists() and (has_plan_change or has_todo_change):
        todo = parse_todo(args.todo)
        next_step_id = todo["current_step_id"]
        next_step_title = todo["current_step_title"]

        if has_plan_change:
            state["current_step_id"] = next_step_id
            state["current_step_title"] = next_step_title
            state["counter"] = 0
            state["code_review_pending"] = False
            state["baseline_sanity_pending"] = False
        elif (
            state["current_step_id"] != next_step_id
            or state["current_step_title"] != next_step_title
        ):
            state["current_step_id"] = next_step_id
            state["current_step_title"] = next_step_title
            state["counter"] = 0
            state["code_review_pending"] = False
            state["baseline_sanity_pending"] = False
        elif not is_lifecycle_commit:
            state["counter"] += 1
            if state["counter"] >= state["review_limit"]:
                state["code_review_pending"] = True

    if not is_lifecycle_commit:
        state["test_baseline_counter"] += 1
        needs_new_baseline = (
            not args.baseline.exists()
            or state["test_baseline_counter"] >= state["test_baseline_limit"]
        )
        if state["test_baseline_counter"] >= state["test_baseline_limit"]:
            state["baseline_sanity_pending"] = True
        if needs_new_baseline:
            refresh_test_baseline(args.baseline, state["test_baseline_regex"])
            state["test_baseline_counter"] = 0

    should_print_code_review = (
        state["code_review_pending"] and not previous_code_review_pending
    )
    should_print_baseline_sanity = (
        state["baseline_sanity_pending"] and not previous_baseline_sanity_pending
    )

    save_state(args.state, state)
    if args.todo.exists():
        sync_todo(args.todo, state)
    if should_print_code_review:
        print(CODE_REVIEW_REMINDER)
    if should_print_baseline_sanity:
        print(BASELINE_SANITY_REMINDER)
    return 0


def build_parser() -> argparse.ArgumentParser:
    todo_path, state_path, baseline_path = default_paths()
    parser = argparse.ArgumentParser()
    parser.add_argument("--todo", type=Path, default=todo_path)
    parser.add_argument("--state", type=Path, default=state_path)
    parser.add_argument("--baseline", type=Path, default=baseline_path)

    subparsers = parser.add_subparsers(dest="command", required=True)

    show = subparsers.add_parser("show")
    show.set_defaults(func=cmd_show)

    reset = subparsers.add_parser("reset")
    reset.add_argument("--step-id")
    reset.add_argument("--step-title")
    reset.set_defaults(func=cmd_reset)

    set_step = subparsers.add_parser("set-step")
    set_step.add_argument("--step-id", required=True)
    set_step.add_argument("--step-title", required=True)
    set_step.set_defaults(func=cmd_set_step)

    increment = subparsers.add_parser("increment")
    increment.set_defaults(func=cmd_increment)

    set_limit = subparsers.add_parser("set-limit")
    set_limit.add_argument("--review-limit", type=int, required=True)
    set_limit.set_defaults(func=cmd_set_limit)

    set_baseline_limit = subparsers.add_parser("set-baseline-limit")
    set_baseline_limit.add_argument("--test-baseline-limit", type=int, required=True)
    set_baseline_limit.set_defaults(func=cmd_set_baseline_limit)

    set_baseline_regex = subparsers.add_parser("set-baseline-regex")
    set_baseline_regex.add_argument("--test-baseline-regex", required=True)
    set_baseline_regex.set_defaults(func=cmd_set_baseline_regex)

    post_commit = subparsers.add_parser("post-commit")
    post_commit.set_defaults(func=cmd_post_commit)

    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
