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


def load_expected_output(src: pathlib.Path) -> str:
    expected = src.with_suffix(".c.expected")
    if expected.exists():
        return expected.read_text(encoding="utf-8")
    return ""


def append_log(path: pathlib.Path, rel: pathlib.Path, details: str) -> None:
    with path.open("a", encoding="utf-8") as f:
        f.write(f"[{rel}]\n")
        if details:
            f.write(details.rstrip() + "\n")
        f.write("\n")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--compiler", required=True)
    ap.add_argument("--clang", default="")
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
    logs_dir = out_root / "logs"
    logs_dir.mkdir(parents=True, exist_ok=True)
    frontend_log = logs_dir / "frontend_fail.log"
    backend_log = logs_dir / "backend_fail.log"
    runtime_log = logs_dir / "runtime_fail.log"
    frontend_log.write_text("", encoding="utf-8")
    backend_log.write_text("", encoding="utf-8")
    runtime_log.write_text("", encoding="utf-8")

    if not args.clang:
        print("[FAIL] --clang is required for c-testsuite end-to-end validation")
        return 1

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
    compile_frontend_failed = 0
    compile_backend_failed = 0
    runtime_failed = 0

    for src in tests:
        rel = src.relative_to(root)
        out_ll = out_root / rel.with_suffix(".ll")
        out_bin = out_root / rel.with_suffix(".bin")
        out_ll.parent.mkdir(parents=True, exist_ok=True)
        out_bin.parent.mkdir(parents=True, exist_ok=True)

        res = compile_one(args.compiler, src, out_ll)
        if res.returncode != 0:
            compile_frontend_failed += 1
            print(f"[FAIL][COMPILE][FRONTEND] {rel}")
            append_log(frontend_log, rel, res.stderr)
            continue

        backend = run([args.clang, str(out_ll), "-o", str(out_bin)])
        if backend.returncode != 0:
            compile_backend_failed += 1
            print(f"[FAIL][COMPILE][BACKEND] {rel}")
            append_log(backend_log, rel, backend.stderr)
            continue

        exe = run([str(out_bin)])
        expected_output = load_expected_output(src)
        actual_output = exe.stdout + exe.stderr

        if exe.returncode != 0:
            runtime_failed += 1
            print(f"[FAIL][RUNTIME][NONZERO_EXIT] {rel}")
            append_log(
                runtime_log,
                rel,
                f"exit={exe.returncode}\nstdout+stderr:\n{actual_output}",
            )
            continue

        if actual_output != expected_output:
            runtime_failed += 1
            print(f"[FAIL][RUNTIME][OUTPUT_MISMATCH] {rel}")
            append_log(
                runtime_log,
                rel,
                "expected:\n"
                + expected_output
                + "\nactual:\n"
                + actual_output
                + f"\nexit={exe.returncode}",
            )
            continue

        passed += 1
        print(f"[PASS] {rel}")

    total = len(tests)
    total_failed = compile_frontend_failed + compile_backend_failed + runtime_failed
    print(
        "[SUMMARY] "
        f"total={total} passed={passed} failed={total_failed} "
        f"compile_frontend={compile_frontend_failed} "
        f"compile_backend={compile_backend_failed} "
        f"runtime={runtime_failed}"
    )
    print(f"[LOG] frontend: {frontend_log}")
    print(f"[LOG] backend:  {backend_log}")
    print(f"[LOG] runtime:  {runtime_log}")

    if total_failed != 0:
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
