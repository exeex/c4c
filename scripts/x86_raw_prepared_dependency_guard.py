#!/usr/bin/env python3
"""Guard x86 MIR migrated surfaces against unclassified raw prepared access."""

from __future__ import annotations

import collections
import re
import sys
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SCAN_ROOT = ROOT / "src/backend/mir/x86"
SOURCE_SUFFIXES = {".cpp", ".hpp"}
HIT_RE = re.compile(r"PreparedBirModule|#include\s+[<\"].*prealloc/")


@dataclass(frozen=True)
class Hit:
    path: str
    line: int
    category: str
    text: str


CLASSIFIED_PATHS: dict[str, str] = {
    # Public and internal entry bridges retained while the public x86 API stays
    # source-compatible and Step 2 exposes a view-taking internal entry.
    "src/backend/mir/x86/api/api.cpp": "compatibility wrapper",
    "src/backend/mir/x86/api/api.hpp": "compatibility wrapper",
    "src/backend/mir/x86/module/module.hpp": "adapter bridge",
    "src/backend/mir/x86/abi/abi.cpp": "compatibility wrapper",
    "src/backend/mir/x86/abi/abi.hpp": "compatibility wrapper",
    "src/backend/mir/x86/codegen/x86_codegen.hpp": "compatibility wrapper",

    # Step 3 migrated one local-slot return path only. The rest of these files
    # still contain tracked per-function/module legacy surfaces until later
    # packets move them behind core or feature views.
    "src/backend/mir/x86/x86.hpp": "tracked legacy surface",
    "src/backend/mir/x86/module/module.cpp": "tracked legacy surface",
    "src/backend/mir/x86/prepared/dispatch.cpp": "tracked legacy surface",
    "src/backend/mir/x86/prepared/prepared.hpp": "tracked legacy surface",

    # Debug/reporting paths may observe legacy prepared-module state but must
    # not become semantic lowering authority.
    "src/backend/mir/x86/codegen/route_debug.hpp": "diagnostic-only rendering",
    "src/backend/mir/x86/debug/debug.cpp": "diagnostic-only rendering",
    "src/backend/mir/x86/debug/debug.hpp": "diagnostic-only rendering",
}

EXPECTED_HIT_COUNTS: dict[str, int] = {
    "src/backend/mir/x86/abi/abi.cpp": 2,
    "src/backend/mir/x86/abi/abi.hpp": 2,
    "src/backend/mir/x86/api/api.cpp": 1,
    "src/backend/mir/x86/api/api.hpp": 1,
    "src/backend/mir/x86/codegen/route_debug.hpp": 2,
    "src/backend/mir/x86/codegen/x86_codegen.hpp": 1,
    "src/backend/mir/x86/debug/debug.cpp": 8,
    "src/backend/mir/x86/debug/debug.hpp": 3,
    "src/backend/mir/x86/module/module.cpp": 60,
    "src/backend/mir/x86/module/module.hpp": 3,
    "src/backend/mir/x86/prepared/dispatch.cpp": 1,
    "src/backend/mir/x86/prepared/prepared.hpp": 4,
    "src/backend/mir/x86/x86.hpp": 15,
}


def iter_source_files() -> list[Path]:
    return sorted(
        path for path in SCAN_ROOT.rglob("*") if path.is_file() and path.suffix in SOURCE_SUFFIXES
    )


def classify_path(rel_path: str) -> str | None:
    return CLASSIFIED_PATHS.get(rel_path)


def scan() -> tuple[list[Hit], list[Hit]]:
    classified: list[Hit] = []
    unclassified: list[Hit] = []
    for path in iter_source_files():
        rel_path = path.relative_to(ROOT).as_posix()
        category = classify_path(rel_path)
        for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            if not HIT_RE.search(line):
                continue
            hit = Hit(
                path=rel_path,
                line=line_number,
                category=category or "unclassified",
                text=line.strip(),
            )
            if category is None:
                unclassified.append(hit)
            else:
                classified.append(hit)
    return classified, unclassified


def count_drift(classified: list[Hit]) -> list[tuple[str, int, int]]:
    actual_counts = collections.Counter(hit.path for hit in classified)
    drifts: list[tuple[str, int, int]] = []
    for path, expected in sorted(EXPECTED_HIT_COUNTS.items()):
        actual = actual_counts.get(path, 0)
        if actual != expected:
            drifts.append((path, expected, actual))
    for path, actual in sorted(actual_counts.items()):
        if path not in EXPECTED_HIT_COUNTS:
            drifts.append((path, 0, actual))
    return drifts


def print_summary(classified: list[Hit],
                  unclassified: list[Hit],
                  drifts: list[tuple[str, int, int]]) -> None:
    counts = collections.Counter(hit.category for hit in classified)
    if unclassified:
        counts["unclassified"] = len(unclassified)
    if drifts:
        counts["path-count-drift"] = len(drifts)

    print("x86 raw prepared dependency guard")
    print(f"scan root: {SCAN_ROOT.relative_to(ROOT).as_posix()}")
    print(f"classified hits: {len(classified)}")
    for category in sorted(counts):
        print(f"  {category}: {counts[category]}")

    paths_by_category: dict[str, set[str]] = collections.defaultdict(set)
    for hit in classified:
        paths_by_category[hit.category].add(hit.path)
    if unclassified:
        for hit in unclassified:
            paths_by_category["unclassified"].add(hit.path)

    print("classified paths:")
    for category in sorted(paths_by_category):
        paths = sorted(paths_by_category[category])
        shown = ", ".join(paths[:8])
        suffix = "" if len(paths) <= 8 else f", ... (+{len(paths) - 8})"
        print(f"  {category}: {shown}{suffix}")

    if unclassified:
        print("unclassified raw dependencies:")
        for hit in unclassified:
            print(f"  {hit.path}:{hit.line}: {hit.text}")
        print("classify new hits as adapter bridge, compatibility wrapper, "
              "tracked legacy surface, or diagnostic-only rendering.")
    if drifts:
        print("classified path hit-count drift:")
        for path, expected, actual in drifts:
            print(f"  {path}: expected {expected}, actual {actual}")
        print("update EXPECTED_HIT_COUNTS only after reviewing the raw dependency change.")


def main() -> int:
    classified, unclassified = scan()
    drifts = count_drift(classified)
    print_summary(classified, unclassified, drifts)
    return 1 if unclassified or drifts else 0


if __name__ == "__main__":
    sys.exit(main())
