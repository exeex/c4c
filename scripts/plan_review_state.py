#!/usr/bin/env python3
import argparse
import json
import re
import subprocess
import sys
from pathlib import Path


CURRENT_STEP_ID_RE = re.compile(r"^Current Step ID:\s*(.*?)\s*$", re.MULTILINE)
CURRENT_STEP_TITLE_RE = re.compile(r"^Current Step Title:\s*(.*?)\s*$", re.MULTILINE)
PLAN_REVIEW_COUNTER_RE = re.compile(
    r"^Plan Review Counter:\s*(\d+)\s*/\s*(\d+)\s*$", re.MULTILINE
)
LEGACY_CURRENT_STEP_RE = re.compile(r"^Step\s+([0-9.]+)\s+(.*?)\s*$")


def repo_root() -> Path:
    return Path(__file__).resolve().parent.parent


def default_paths():
    root = repo_root()
    return root / "todo.md", root / ".plan_review_state.json"


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def write_text(path: Path, text: str) -> None:
    path.write_text(text, encoding="utf-8")


def parse_todo(path: Path) -> dict:
    text = read_text(path)
    step_id_match = CURRENT_STEP_ID_RE.search(text)
    step_title_match = CURRENT_STEP_TITLE_RE.search(text)
    counter_match = PLAN_REVIEW_COUNTER_RE.search(text)
    if not step_id_match:
        raise SystemExit(f"missing 'Current Step ID:' line in {path}")
    if not step_title_match:
        raise SystemExit(f"missing 'Current Step Title:' line in {path}")
    if not counter_match:
        raise SystemExit(f"missing 'Plan Review Counter:' line in {path}")
    return {
        "text": text,
        "current_step_id": step_id_match.group(1),
        "current_step_title": step_title_match.group(1),
        "counter": int(counter_match.group(1)),
        "review_limit": int(counter_match.group(2)),
    }


def replace_once(text: str, pattern: re.Pattern, replacement: str) -> str:
    new_text, count = pattern.subn(replacement, text, count=1)
    if count != 1:
        raise SystemExit("expected exactly one metadata line to replace in todo.md")
    return new_text


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
    text = replace_once(
        text,
        PLAN_REVIEW_COUNTER_RE,
        f"Plan Review Counter: {state['counter']} / {state['review_limit']}",
    )
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
    return data


def save_state(path: Path, state: dict) -> None:
    payload = {
        "current_step_id": state["current_step_id"],
        "current_step_title": state["current_step_title"],
        "counter": state["counter"],
        "review_limit": state["review_limit"],
    }
    write_text(path, json.dumps(payload, indent=2, sort_keys=True) + "\n")


def init_from_todo(todo_path: Path, state_path: Path) -> dict:
    parsed = parse_todo(todo_path)
    state = {
        "current_step_id": parsed["current_step_id"],
        "current_step_title": parsed["current_step_title"],
        "counter": parsed["counter"],
        "review_limit": parsed["review_limit"],
    }
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


def staged_paths() -> set[str]:
    output = git("diff", "--cached", "--name-only", "--diff-filter=ACMR")
    return {line.strip() for line in output.splitlines() if line.strip()}


def git_add(*paths: Path) -> None:
    rel_paths = [str(path.relative_to(repo_root())) for path in paths]
    subprocess.run(["git", "add", *rel_paths], check=True, cwd=repo_root())


def ensure_state(todo_path: Path, state_path: Path) -> dict:
    state = load_state(state_path)
    if state is None:
        state = init_from_todo(todo_path, state_path)
    return state


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
    save_state(args.state, state)
    sync_todo(args.todo, state)
    return 0


def cmd_increment(args) -> int:
    state = ensure_state(args.todo, args.state)
    state["counter"] += 1
    save_state(args.state, state)
    sync_todo(args.todo, state)
    return 0


def cmd_set_limit(args) -> int:
    state = ensure_state(args.todo, args.state)
    state["review_limit"] = args.review_limit
    save_state(args.state, state)
    sync_todo(args.todo, state)
    return 0


def cmd_prepare_commit(args) -> int:
    if not args.todo.exists():
        return 0

    staged = staged_paths()
    has_plan_change = "plan.md" in staged
    has_todo_change = "todo.md" in staged

    # Pure idea-only commits should not rewrite or stage todo metadata.
    if not has_plan_change and not has_todo_change:
        return 0

    state = ensure_state(args.todo, args.state)
    todo = parse_todo(args.todo)

    next_step_id = todo["current_step_id"]
    next_step_title = todo["current_step_title"]

    if has_plan_change:
        state["current_step_id"] = next_step_id
        state["current_step_title"] = next_step_title
        state["counter"] = 0
    elif (
        state["current_step_id"] != next_step_id
        or state["current_step_title"] != next_step_title
    ):
        state["current_step_id"] = next_step_id
        state["current_step_title"] = next_step_title
        state["counter"] = 0
    else:
        state["counter"] += 1

    save_state(args.state, state)
    sync_todo(args.todo, state)
    git_add(args.todo)
    return 0


def build_parser() -> argparse.ArgumentParser:
    todo_path, state_path = default_paths()
    parser = argparse.ArgumentParser()
    parser.add_argument("--todo", type=Path, default=todo_path)
    parser.add_argument("--state", type=Path, default=state_path)

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

    prepare = subparsers.add_parser("prepare-commit")
    prepare.set_defaults(func=cmd_prepare_commit)

    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
