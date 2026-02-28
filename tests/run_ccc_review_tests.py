#!/usr/bin/env python3
import argparse
import pathlib
import subprocess
import sys


def run(cmd: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(cmd, text=True, capture_output=True, encoding="utf-8", errors="replace")


def parse_expect(src: pathlib.Path) -> str:
    # Default to pass if not annotated.
    expect = "pass"
    for line in src.read_text(encoding="utf-8").splitlines()[:20]:
        s = line.strip()
        if s.startswith("// CCC_EXPECT:"):
            expect = s.split(":", 1)[1].strip().lower()
            break
    return "fail" if expect == "fail" else "pass"


def run_case(compiler: str, clang: str, src: pathlib.Path, out_root: pathlib.Path) -> tuple[str, str]:
    expect = parse_expect(src)
    rel = src.name

    clang_bin = out_root / f"{src.stem}.clang.bin"
    ll_path = out_root / f"{src.stem}.ll"
    c2ll_bin = out_root / f"{src.stem}.c2ll.bin"

    clang_build = run([clang, "-std=gnu11", "-fdollars-in-identifiers", str(src), "-o", str(clang_bin)])
    if clang_build.returncode != 0:
        return "fail", f"[FAIL][CLANG][COMPILE] {rel}\n{clang_build.stderr}"

    clang_run = run([str(clang_bin)])
    if clang_run.returncode != 0:
        return "fail", (
            f"[FAIL][CLANG][RUNTIME] {rel}\n"
            f"exit={clang_run.returncode}\n"
            f"stdout+stderr:\n{clang_run.stdout}{clang_run.stderr}"
        )

    front = run([sys.executable, compiler, str(src), "-o", str(ll_path)])
    if front.returncode != 0:
        if expect == "fail":
            return "pass", f"[PASS][XFAIL][FRONTEND] {rel}"
        return "fail", f"[FAIL][FRONTEND] {rel}\n{front.stderr}"

    back = run([clang, str(ll_path), "-o", str(c2ll_bin)])
    if back.returncode != 0:
        if expect == "fail":
            return "pass", f"[PASS][XFAIL][BACKEND] {rel}"
        return "fail", f"[FAIL][BACKEND] {rel}\n{back.stderr}"

    c2ll_run = run([str(c2ll_bin)])
    actual = c2ll_run.stdout + c2ll_run.stderr
    expected = clang_run.stdout + clang_run.stderr
    matched = (c2ll_run.returncode == clang_run.returncode) and (actual == expected)

    if expect == "fail":
        if matched:
            return "fail", f"[FAIL][XPASS] {rel} (expected fail but matched clang)"
        return "pass", f"[PASS][XFAIL][RUNTIME] {rel}"

    if not matched:
        return "fail", (
            f"[FAIL][RUNTIME] {rel}\n"
            f"clang_exit={clang_run.returncode} c2ll_exit={c2ll_run.returncode}\n"
            f"clang_out:\n{expected}\n"
            f"c2ll_out:\n{actual}"
        )

    return "pass", f"[PASS] {rel}"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--compiler", required=True)
    ap.add_argument("--clang", required=True)
    ap.add_argument("--tests-root", required=True)
    ap.add_argument("--workdir", required=True)
    args = ap.parse_args()

    tests_root = pathlib.Path(args.tests_root).resolve()
    workdir = pathlib.Path(args.workdir).resolve() / "ccc_review_tests"
    workdir.mkdir(parents=True, exist_ok=True)

    tests = sorted(tests_root.glob("*.c"))
    if not tests:
        print("[SKIP] no ccc-review tests found")
        return 0

    failed = 0
    for src in tests:
        status, msg = run_case(args.compiler, args.clang, src, workdir)
        print(msg)
        if status != "pass":
            failed += 1

    total = len(tests)
    print(f"[SUMMARY] total={total} passed={total - failed} failed={failed}")
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())

