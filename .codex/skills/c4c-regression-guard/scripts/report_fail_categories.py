#!/usr/bin/env python3
"""Generate failed testcase report grouped by c4c failure categories."""

from __future__ import annotations

import argparse
import json
import re
import sys
from collections import OrderedDict
from pathlib import Path


FAILED_TEST_RE = re.compile(
    r"^\s*\d+/\d+\s+Test\s+#\d+:\s+(?P<name>.*?)\s+\.+\*\*\*Failed\s+[0-9]+(?:\.[0-9]+)?\s+sec$"
)
CATEGORY_RE = re.compile(r"\[(FRONTEND_FAIL|BACKEND_FAIL|RUNTIME_FAIL|RUNTIME_MISMATCH)\]")


def normalize_category(raw: str) -> str:
    if raw == "RUNTIME_MISMATCH":
        return "RUNTIME_FAIL"
    return raw


def parse_failure_categories(path: Path) -> dict[str, list[str]]:
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    failed_order: list[str] = []
    pending_tests: list[str] = []
    assigned: dict[str, str] = {}

    for line in lines:
        m_test = FAILED_TEST_RE.match(line)
        if m_test:
            test_name = m_test.group("name").strip()
            failed_order.append(test_name)
            pending_tests.append(test_name)
            continue

        m_cat = CATEGORY_RE.search(line)
        if m_cat and pending_tests:
            category = normalize_category(m_cat.group(1))
            test_name = pending_tests.pop(0)
            assigned[test_name] = category

    groups: dict[str, list[str]] = OrderedDict(
        {
            "FRONTEND_FAIL": [],
            "BACKEND_FAIL": [],
            "RUNTIME_FAIL": [],
            "UNKNOWN": [],
        }
    )
    seen = {key: set() for key in groups}
    for test_name in failed_order:
        category = assigned.get(test_name, "UNKNOWN")
        if test_name not in seen[category]:
            groups[category].append(test_name)
            seen[category].add(test_name)

    return groups


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--log", required=True, type=Path, help="Path to ctest output log")
    parser.add_argument(
        "--format",
        choices=("text", "json"),
        default="text",
        help="Output format (default: text).",
    )
    args = parser.parse_args()

    if not args.log.is_file():
        print(f"ERROR: log not found: {args.log}", file=sys.stderr)
        return 2

    groups = parse_failure_categories(args.log)
    total_failed = sum(len(v) for v in groups.values())

    if args.format == "json":
        payload = {
            "log": str(args.log),
            "total_failed": total_failed,
            "categories": groups,
        }
        print(json.dumps(payload, ensure_ascii=False, indent=2))
        return 0

    print(f"log: {args.log}")
    print(f"total failed testcases: {total_failed}")
    for category in ("FRONTEND_FAIL", "BACKEND_FAIL", "RUNTIME_FAIL"):
        names = groups[category]
        print(f"{category}: {len(names)}")
        for name in names:
            print(f"  - {name}")
    if groups["UNKNOWN"]:
        print(f"UNKNOWN: {len(groups['UNKNOWN'])}")
        for name in groups["UNKNOWN"]:
            print(f"  - {name}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
