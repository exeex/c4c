#!/usr/bin/env python3

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
DEFAULT_OBJECT_ROOT = ROOT / "build" / "CMakeFiles" / "c4cll.dir" / "src"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Report ELF object sizes by section for .o files."
    )
    parser.add_argument(
        "object_root",
        nargs="?",
        type=Path,
        default=DEFAULT_OBJECT_ROOT,
        help=f"Root directory to scan for .o files (default: {DEFAULT_OBJECT_ROOT})",
    )
    parser.add_argument(
        "--section",
        default=".text",
        help="ELF section name to report (default: .text)",
    )
    parser.add_argument(
        "--limit",
        type=int,
        default=0,
        help="Show only the largest N entries (default: show all)",
    )
    parser.add_argument(
        "--human",
        action="store_true",
        help="Print sizes in human-readable units",
    )
    return parser.parse_args()


def read_section_size(path: Path, section_name: str) -> int:
    result = subprocess.run(
        ["readelf", "-S", "--wide", str(path)],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or "readelf failed")

    for line in result.stdout.splitlines():
        parts = line.split()
        if len(parts) < 7:
            continue
        if parts[1] != section_name:
            continue
        return int(parts[5], 16)

    return 0


def format_size(size: int, human: bool) -> str:
    if not human:
        return str(size)

    units = ["B", "KiB", "MiB", "GiB"]
    value = float(size)
    unit = units[0]
    for unit in units:
        if value < 1024.0 or unit == units[-1]:
            break
        value /= 1024.0
    return f"{value:.2f} {unit}"


def main() -> int:
    args = parse_args()
    object_root = args.object_root.resolve()

    if not object_root.is_dir():
        print(f"error: directory not found: {object_root}", file=sys.stderr)
        return 1

    object_files = sorted(object_root.rglob("*.o"))
    if not object_files:
        print(f"error: no .o files found under {object_root}", file=sys.stderr)
        return 1

    entries: list[tuple[int, Path]] = []
    failures: list[tuple[Path, str]] = []

    for path in object_files:
        try:
            size = read_section_size(path, args.section)
        except RuntimeError as exc:
            failures.append((path, str(exc)))
            continue
        entries.append((size, path))

    entries.sort(key=lambda item: (item[0], str(item[1])), reverse=True)
    total_size = sum(size for size, _ in entries)
    if args.limit > 0:
        entries = entries[: args.limit]

    for size, path in entries:
        rel_path = path.relative_to(ROOT) if path.is_relative_to(ROOT) else path
        print(f"{format_size(size, args.human):>10}  {rel_path}")

    print(f"\nTotal {args.section}: {format_size(total_size, args.human)}")
    print(f"Files listed: {len(entries)}")

    if failures:
        print(f"\nWarnings: failed to inspect {len(failures)} files", file=sys.stderr)
        for path, message in failures[:10]:
            print(f"  {path}: {message}", file=sys.stderr)
        if len(failures) > 10:
            print("  ...", file=sys.stderr)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
