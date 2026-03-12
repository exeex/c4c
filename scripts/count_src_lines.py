#!/usr/bin/env python3

from __future__ import annotations

from collections import defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
SRC_DIR = ROOT / "src"
EXTENSIONS = ("*.cpp", "*.hpp")


def count_lines(path: Path) -> int:
    with path.open("r", encoding="utf-8", errors="ignore") as handle:
        return sum(1 for _ in handle)


def main() -> None:
    files = sorted(
        {
            path
            for pattern in EXTENSIONS
            for path in SRC_DIR.rglob(pattern)
            if path.is_file()
        }
    )

    if not files:
        print("No .cpp or .hpp files found under src/")
        return

    dir_totals: dict[str, int] = defaultdict(int)
    total_lines = 0

    print("Per file:")
    for path in files:
        rel_path = path.relative_to(ROOT)
        lines = count_lines(path)
        total_lines += lines
        dir_totals[str(rel_path.parent)] += lines
        print(f"{lines:6d}  {rel_path}")

    print("\nPer directory:")
    for directory, lines in sorted(dir_totals.items(), key=lambda item: (-item[1], item[0])):
        print(f"{lines:6d}  {directory}/")

    print(f"\nTotal: {total_lines} lines across {len(files)} files")


if __name__ == "__main__":
    main()
