#!/usr/bin/env python3
"""
ops/orchestrator.py — Local Git-Triggered Multi-Agent Orchestrator
===================================================================
Layers:
  hook  -> enqueue-event   : detect Git changes, write domain events to queue
  daemon-> process-queue   : read queue, route events to agent handlers
  agent -> handle_*        : create plan branches, initialize work, log state

Usage:
  python3 ops/orchestrator.py enqueue-event post-commit --head HEAD
  python3 ops/orchestrator.py process-queue
  python3 ops/orchestrator.py show-queue
  python3 ops/orchestrator.py show-log
"""

import argparse
import json
import os
import re
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
REPO_ROOT = Path(__file__).resolve().parent.parent
OPS_DIR = REPO_ROOT / "ops"
STATE_DIR = OPS_DIR / "state"
EXECUTION_DIR = OPS_DIR / "execution"
QUEUE_FILE = STATE_DIR / "event_queue.jsonl"
EVENT_LOG = STATE_DIR / "events.jsonl"
ORCH_LOG = STATE_DIR / "orchestrator.log"
RULES_FILE = OPS_DIR / "rules.json"

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _now() -> str:
    return datetime.now(timezone.utc).isoformat()


def _run(cmd: list[str], **kwargs) -> subprocess.CompletedProcess:
    kwargs.setdefault("cwd", REPO_ROOT)
    kwargs.setdefault("check", True)
    kwargs.setdefault("text", True)
    kwargs.setdefault("capture_output", True)
    return subprocess.run(cmd, **kwargs)


def _log(msg: str, file=None):
    line = f"[{_now()}] {msg}"
    print(line, file=file or sys.stdout)
    if file is None:
        STATE_DIR.mkdir(parents=True, exist_ok=True)
        with ORCH_LOG.open("a") as f:
            f.write(line + "\n")


def _log_event(event: dict, status: str, extra: dict | None = None):
    STATE_DIR.mkdir(parents=True, exist_ok=True)
    entry = {**event, "status": status, "logged_at": _now()}
    if extra:
        entry.update(extra)
    with EVENT_LOG.open("a") as f:
        f.write(json.dumps(entry) + "\n")


def _load_rules() -> dict:
    if RULES_FILE.exists():
        return json.loads(RULES_FILE.read_text())
    return {}


# ---------------------------------------------------------------------------
# Git helpers
# ---------------------------------------------------------------------------

def _current_branch() -> str:
    return _run(["git", "rev-parse", "--abbrev-ref", "HEAD"]).stdout.strip()


def _get_changed_files(head: str) -> list[str]:
    """Return lines of 'STATUS\tpath' for the given commit."""
    result = _run(
        ["git", "diff-tree", "--no-commit-id", "--name-status", "-r", head]
    )
    return result.stdout.strip().splitlines()


def _branch_exists(branch: str) -> bool:
    result = subprocess.run(
        ["git", "branch", "--list", branch],
        cwd=REPO_ROOT, capture_output=True, text=True
    )
    return bool(result.stdout.strip())


def _remote_branch_exists(branch: str) -> bool:
    result = subprocess.run(
        ["git", "branch", "-r", "--list", f"origin/{branch}"],
        cwd=REPO_ROOT, capture_output=True, text=True
    )
    return bool(result.stdout.strip())


def _next_plan_number() -> str:
    """Find the highest PLAN-NNN in local + remote branches and return next."""
    result = subprocess.run(
        ["git", "branch", "-a"],
        cwd=REPO_ROOT, capture_output=True, text=True
    )
    nums = [int(m) for m in re.findall(r"PLAN-(\d+)", result.stdout)]
    return f"{(max(nums, default=0) + 1):03d}"


def _default_base_branch() -> str:
    """Return 'main' if it exists, else 'master'."""
    result = subprocess.run(
        ["git", "branch", "--list", "main"],
        cwd=REPO_ROOT, capture_output=True, text=True
    )
    return "main" if result.stdout.strip() else "master"


# ---------------------------------------------------------------------------
# Event detection
# ---------------------------------------------------------------------------

def _detect_events(changed_files: list[str]) -> list[dict]:
    events = []
    for line in changed_files:
        parts = line.split("\t", 1)
        if len(parts) != 2:
            continue
        status, path = parts
        # NewIdeaCreated: new .md added under ideas/open/
        if status == "A" and path.startswith("ideas/open/") and path.endswith(".md"):
            idea_name = Path(path).stem
            events.append({
                "type": "NewIdeaCreated",
                "idea": idea_name,
                "path": path,
                "timestamp": _now(),
            })
        # PlanCompleted: plan_todo.md modified and all items checked
        if status in ("M", "A") and Path(path).name == "plan_todo.md":
            full = REPO_ROOT / path
            if full.exists():
                text = full.read_text()
                open_items = re.findall(r"- \[ \]", text)
                closed_items = re.findall(r"- \[x\]", text, re.IGNORECASE)
                if closed_items and not open_items:
                    events.append({
                        "type": "PlanCompleted",
                        "path": path,
                        "timestamp": _now(),
                    })
    return events


# ---------------------------------------------------------------------------
# Queue management
# ---------------------------------------------------------------------------

def _enqueue(events: list[dict]):
    STATE_DIR.mkdir(parents=True, exist_ok=True)
    with QUEUE_FILE.open("a") as f:
        for e in events:
            f.write(json.dumps(e) + "\n")


def _dequeue_one() -> dict | None:
    if not QUEUE_FILE.exists():
        return None
    lines = QUEUE_FILE.read_text().strip().splitlines()
    if not lines:
        return None
    event = json.loads(lines[0])
    remaining = lines[1:]
    QUEUE_FILE.write_text("\n".join(remaining) + ("\n" if remaining else ""))
    return event


# ---------------------------------------------------------------------------
# Agent handlers
# ---------------------------------------------------------------------------

def _handle_new_idea_created(event: dict):
    idea_name = event["idea"]
    idea_path = event["path"]
    _log(f"NewIdeaCreated -> planner agent for '{idea_name}'")
    _log_event(event, "planner_assigned")

    base = _default_base_branch()
    plan_num = _next_plan_number()
    branch = f"plan/PLAN-{plan_num}-{idea_name}"

    orig_branch = _current_branch()
    try:
        # Create plan branch from base
        _run(["git", "checkout", "-b", branch, base])
        _log(f"created branch: {branch}")

        # Write plan.md
        idea_full = REPO_ROOT / idea_path
        idea_content = idea_full.read_text() if idea_full.exists() else f"# {idea_name}\n"
        plan_md = (
            f"# Plan: {idea_name}\n\n"
            f"Source: `{idea_path}`\n\n"
            f"## Idea Summary\n\n"
            f"{idea_content[:800]}\n\n"
            f"## Execution Notes\n\n"
            f"_Agent work notes go here._\n"
        )
        (REPO_ROOT / "plan.md").write_text(plan_md)

        # Write plan_todo.md
        todo_md = (
            f"# TODO: {idea_name}\n\n"
            f"- [ ] Review idea and decompose tasks\n"
            f"- [ ] Implement changes\n"
            f"- [ ] Write / update tests\n"
            f"- [ ] Run validation checks\n"
            f"- [ ] Mark plan complete and merge to {base}\n"
        )
        (REPO_ROOT / "plan_todo.md").write_text(todo_md)

        # Create execution subdir
        exec_dir = EXECUTION_DIR / f"PLAN-{plan_num}"
        exec_dir.mkdir(parents=True, exist_ok=True)
        (exec_dir / "notes.md").write_text(
            f"# Execution Notes: PLAN-{plan_num}\n\n"
            f"Idea: `{idea_path}`\n"
            f"Branch: `{branch}`\n"
            f"Created: {_now()}\n"
        )

        _run(["git", "add", "plan.md", "plan_todo.md",
              str(exec_dir.relative_to(REPO_ROOT))])
        _run(["git", "commit", "-m",
              f"plan: init PLAN-{plan_num} for {idea_name}"])

        _log(f"plan branch ready: {branch}")
        _log_event(event, "plan_branch_ready", {"branch": branch, "plan": f"PLAN-{plan_num}"})

    except subprocess.CalledProcessError as exc:
        _log(f"ERROR in _handle_new_idea_created: {exc.stderr or exc}")
        _log_event(event, "error", {"error": str(exc)})
    finally:
        # Return to original branch regardless
        subprocess.run(["git", "checkout", orig_branch],
                       cwd=REPO_ROOT, capture_output=True)


def _handle_plan_completed(event: dict):
    _log("PlanCompleted -> validation / merge agent (not yet implemented)")
    _log_event(event, "validation_pending")
    # Phase 2: trigger merge/validation agent


def _route_event(event: dict):
    rules = _load_rules()
    etype = event.get("type")

    # Check rules override
    if etype in rules.get("routes", {}):
        handler_name = rules["routes"][etype]
        _log(f"routing {etype} via rules -> {handler_name}")

    if etype == "NewIdeaCreated":
        _handle_new_idea_created(event)
    elif etype == "PlanCompleted":
        _handle_plan_completed(event)
    else:
        _log(f"unknown event type: {etype}")
        _log_event(event, "unhandled")


# ---------------------------------------------------------------------------
# CLI commands
# ---------------------------------------------------------------------------

def cmd_enqueue_event(args):
    """Called by the post-commit hook."""
    head = args.head
    _log(f"enqueue-event: inspecting commit {head[:12]}")
    changed = _get_changed_files(head)
    events = _detect_events(changed)
    if not events:
        _log("no domain events detected")
        return
    _enqueue(events)
    for e in events:
        _log(f"enqueued: {e['type']}({e.get('idea', e.get('path', ''))})")

    # Spawn process-queue asynchronously so the hook returns immediately
    STATE_DIR.mkdir(parents=True, exist_ok=True)
    with ORCH_LOG.open("a") as log_fh:
        subprocess.Popen(
            [sys.executable, __file__, "process-queue"],
            cwd=REPO_ROOT,
            stdout=log_fh,
            stderr=log_fh,
            start_new_session=True,
        )


def cmd_process_queue(_args):
    """Drain the event queue, routing each event to the correct handler."""
    processed = 0
    while True:
        event = _dequeue_one()
        if event is None:
            break
        _log(f"processing: {event['type']}")
        _route_event(event)
        processed += 1
    _log(f"process-queue done ({processed} events)")


def cmd_show_queue(_args):
    if not QUEUE_FILE.exists() or not QUEUE_FILE.read_text().strip():
        print("Queue is empty.")
        return
    for line in QUEUE_FILE.read_text().strip().splitlines():
        print(line)


def cmd_show_log(_args):
    if not EVENT_LOG.exists():
        print("No event log yet.")
        return
    for line in EVENT_LOG.read_text().strip().splitlines():
        try:
            e = json.loads(line)
            print(f"{e.get('logged_at','?')[:19]}  {e.get('type','?'):<25} {e.get('status','?'):<20} {e.get('idea', e.get('branch', ''))}")
        except json.JSONDecodeError:
            print(line)


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description="c4c orchestrator")
    sub = parser.add_subparsers(dest="command")

    p_enqueue = sub.add_parser("enqueue-event")
    p_enqueue.add_argument("trigger", help="e.g. post-commit")
    p_enqueue.add_argument("--head", default="HEAD")

    sub.add_parser("process-queue")
    sub.add_parser("show-queue")
    sub.add_parser("show-log")

    args = parser.parse_args()
    if args.command == "enqueue-event":
        cmd_enqueue_event(args)
    elif args.command == "process-queue":
        cmd_process_queue(args)
    elif args.command == "show-queue":
        cmd_show_queue(args)
    elif args.command == "show-log":
        cmd_show_log(args)
    else:
        parser.print_help()


if __name__ == "__main__":
    main()
