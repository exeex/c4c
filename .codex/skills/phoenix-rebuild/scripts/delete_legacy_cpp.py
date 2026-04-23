#!/usr/bin/env python3
"""Delete legacy .cpp files after in-place markdown extraction exists."""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


SKILL_ROOT = Path(__file__).resolve().parent.parent
REPO_ROOT = SKILL_ROOT.parent.parent.parent


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Delete legacy .cpp files only after matching markdown evidence exists.",
    )
    parser.add_argument(
        "patterns",
        nargs="+",
        help="Glob patterns relative to the repo root, for example src/backend/mir/x86/codegen/*.cpp",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Show which files would be removed without deleting them.",
    )
    return parser.parse_args()


def expand_cpp_patterns(patterns: list[str]) -> list[Path]:
    matches: set[Path] = set()
    for pattern in patterns:
        for match in REPO_ROOT.glob(pattern):
            if match.is_file() and match.suffix == ".cpp":
                matches.add(match.resolve())
    return sorted(matches)


def required_markdown_path(source_path: Path) -> Path:
    return source_path.with_name(source_path.name + ".md")


def main() -> int:
    args = parse_args()
    cpp_paths = expand_cpp_patterns(args.patterns)
    if not cpp_paths:
        print("No .cpp files matched the requested glob patterns.", file=sys.stderr)
        return 1

    missing_markdown: list[tuple[Path, Path]] = []
    for cpp_path in cpp_paths:
        markdown_path = required_markdown_path(cpp_path)
        if not markdown_path.is_file():
            missing_markdown.append((cpp_path, markdown_path))

    if missing_markdown:
        print("Refusing to delete legacy .cpp without matching markdown evidence:", file=sys.stderr)
        for cpp_path, markdown_path in missing_markdown:
            print(
                f"- missing {markdown_path.relative_to(REPO_ROOT)} for {cpp_path.relative_to(REPO_ROOT)}",
                file=sys.stderr,
            )
        return 1

    rm_command = ["rm", *[str(path) for path in cpp_paths]]
    if args.dry_run:
        print("Dry run. Would execute:")
        print(" ".join(rm_command))
        return 0

    subprocess.run(rm_command, cwd=REPO_ROOT, check=True)
    for cpp_path in cpp_paths:
        print(f"deleted {cpp_path.relative_to(REPO_ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
