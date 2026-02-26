#!/usr/bin/env python3
import argparse
import pathlib
import subprocess
import sys


def run(cmd: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(cmd, text=True, capture_output=True)


def load_allowlist(path: pathlib.Path) -> list[str]:
    if not path.exists():
        return []
    entries: list[str] = []
    for line in path.read_text(encoding="utf-8").splitlines():
        s = line.strip()
        if not s or s.startswith("#"):
            continue
        entries.append(s)
    return entries


def discover_tests(root: pathlib.Path, allowlist: list[str]) -> list[pathlib.Path]:
    if allowlist:
        tests: list[pathlib.Path] = []
        for rel in allowlist:
            p = root / rel
            if p.exists() and p.suffix == ".c":
                tests.append(p)
            else:
                print(f"[WARN] allowlist entry not found or not .c: {rel}")
        return tests
    return sorted(root.rglob("*.c"))


def compile_one(compiler: str, src: pathlib.Path, out_ll: pathlib.Path) -> subprocess.CompletedProcess[str]:
    return run([sys.executable, compiler, str(src), "-o", str(out_ll)])


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--compiler", required=True)
    ap.add_argument("--testsuite-root", required=True)
    ap.add_argument("--workdir", required=True)
    ap.add_argument("--allowlist", default="")
    ap.add_argument("--max-tests", type=int, default=0)
    args = ap.parse_args()

    root = pathlib.Path(args.testsuite_root).resolve()
    if not root.exists():
        print(f"[FAIL] testsuite root does not exist: {root}")
        return 1

    workdir = pathlib.Path(args.workdir).resolve()
    out_root = workdir / "c_testsuite"
    out_root.mkdir(parents=True, exist_ok=True)

    allowlist_entries: list[str] = []
    if args.allowlist:
        allowlist_entries = load_allowlist(pathlib.Path(args.allowlist).resolve())

    tests = discover_tests(root, allowlist_entries)
    if args.max_tests > 0:
        tests = tests[: args.max_tests]

    if not tests:
        print("[SKIP] no c-testsuite cases selected")
        return 0

    passed = 0
    failed = 0

    for src in tests:
        rel = src.relative_to(root)
        out_ll = out_root / rel.with_suffix(".ll")
        out_ll.parent.mkdir(parents=True, exist_ok=True)

        res = compile_one(args.compiler, src, out_ll)
        if res.returncode == 0:
            passed += 1
            print(f"[PASS] {rel}")
        else:
            failed += 1
            print(f"[FAIL] {rel}")
            stderr = res.stderr.strip()
            if stderr:
                print(stderr.splitlines()[-1])

    print(f"[SUMMARY] total={len(tests)} passed={passed} failed={failed}")
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
