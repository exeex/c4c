#!/usr/bin/env python3
"""Check c4c regression monotonicity from before/after CTest logs."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


SUMMARY_RE = re.compile(
    r"(?P<passed_pct>\d+)% tests passed, (?P<failed>\d+) tests failed out of (?P<total>\d+)"
)
FAILED_LIST_RE = re.compile(r"^\s*\d+\s*-\s*(?P<name>[A-Za-z0-9_.+-]+)\s+\(Failed\)\s+")
RESULT_LINE_RE = re.compile(
    r"^\s*\d+/\d+\s+Test\s+#\d+:\s+(?P<name>.*?)\s+\.{3,}.*?(?P<seconds>[0-9]+(?:\.[0-9]+)?)\s+sec$"
)


class ParseError(Exception):
    pass


def parse_log(path: Path) -> dict:
    text = path.read_text(encoding="utf-8", errors="replace")
    lines = text.splitlines()

    summary_match = None
    for line in lines:
        m = SUMMARY_RE.search(line)
        if m:
            summary_match = m
    if summary_match is None:
        raise ParseError(f"Could not find summary line in {path}")

    failed_tests = set()
    durations = {}
    for line in lines:
        m_fail = FAILED_LIST_RE.match(line)
        if m_fail:
            failed_tests.add(m_fail.group("name"))
            continue

        m_result = RESULT_LINE_RE.match(line)
        if m_result:
            durations[m_result.group("name").strip()] = float(m_result.group("seconds"))

    total = int(summary_match.group("total"))
    failed = int(summary_match.group("failed"))
    passed = total - failed

    return {
        "total": total,
        "failed": failed,
        "passed": passed,
        "failed_tests": failed_tests,
        "durations": durations,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--before", required=True, type=Path, help="Path to before log")
    parser.add_argument("--after", required=True, type=Path, help="Path to after log")
    parser.add_argument(
        "--allow-non-decreasing-passed",
        action="store_true",
        help="Allow equal passed count (default requires strict increase).",
    )
    parser.add_argument(
        "--timeout-threshold",
        type=float,
        default=30.0,
        help="Threshold in seconds for suspicious runtime policy (default: 30).",
    )
    parser.add_argument(
        "--enforce-timeout",
        action="store_true",
        help="Fail when new suspicious-timeout tests are introduced.",
    )
    args = parser.parse_args()

    if not args.before.is_file():
        print(f"ERROR: before log not found: {args.before}", file=sys.stderr)
        return 2
    if not args.after.is_file():
        print(f"ERROR: after log not found: {args.after}", file=sys.stderr)
        return 2

    try:
        before = parse_log(args.before)
        after = parse_log(args.after)
    except ParseError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2

    violations = []

    new_failures = sorted(after["failed_tests"] - before["failed_tests"])
    resolved_failures = sorted(before["failed_tests"] - after["failed_tests"])

    if new_failures:
        violations.append("New failing tests introduced.")

    if args.allow_non_decreasing_passed:
        if after["passed"] < before["passed"]:
            violations.append("Passed count decreased.")
    else:
        if after["passed"] <= before["passed"]:
            violations.append("Passed count did not strictly increase.")

    before_slow = {
        name for name, secs in before["durations"].items() if secs > args.timeout_threshold
    }
    after_slow = {
        name for name, secs in after["durations"].items() if secs > args.timeout_threshold
    }
    new_slow = sorted(after_slow - before_slow)

    print("c4c regression guard report")
    print(f"before: passed={before['passed']} failed={before['failed']} total={before['total']}")
    print(f"after : passed={after['passed']} failed={after['failed']} total={after['total']}")
    print(f"delta : passed={after['passed'] - before['passed']} failed={after['failed'] - before['failed']}")

    print(f"resolved failing tests: {len(resolved_failures)}")
    if resolved_failures:
        for name in resolved_failures[:20]:
            print(f"  + {name}")
        if len(resolved_failures) > 20:
            print(f"  ... and {len(resolved_failures) - 20} more")

    print(f"new failing tests: {len(new_failures)}")
    if new_failures:
        for name in new_failures:
            print(f"  - {name}")

    print(f"new >{args.timeout_threshold:.2f}s tests: {len(new_slow)}")
    if new_slow:
        for name in new_slow:
            print(f"  ! {name}")
        if args.enforce_timeout:
            violations.append("New suspicious-timeout tests introduced.")

    if violations:
        print("result: FAIL")
        for msg in violations:
            print(f"reason: {msg}")
        return 1

    print("result: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
