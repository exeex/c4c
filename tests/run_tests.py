#!/usr/bin/env python3
import argparse
import pathlib
import subprocess
import sys


def run(cmd: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(cmd, text=True, capture_output=True)

def compiler_cmd(compiler: str, src: pathlib.Path, out_ll: pathlib.Path | None = None) -> list[str]:
    if compiler.endswith(".py"):
        cmd = [sys.executable, compiler, str(src)]
    else:
        cmd = [compiler, str(src)]
    if out_ll is not None:
        cmd.extend(["-o", str(out_ll)])
    return cmd


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--compiler", required=True)
    ap.add_argument("--workdir", required=True)
    ap.add_argument("--clang", default="")
    args = ap.parse_args()

    root = pathlib.Path(__file__).resolve().parent
    workdir = pathlib.Path(args.workdir)
    workdir.mkdir(parents=True, exist_ok=True)

    example_c = root / "example.c"
    bad_c = root / "bad_uninitialized.c"
    out_ll = workdir / "test_example.ll"

    compile_ok = run(compiler_cmd(args.compiler, example_c, out_ll))
    if compile_ok.returncode != 0:
        print("[FAIL] compile example.c")
        print(compile_ok.stderr)
        return 1

    ll_text = out_ll.read_text(encoding="utf-8")
    if "define i32 @main()" not in ll_text:
        print("[FAIL] generated IR missing main definition")
        return 1
    print("[PASS] compile example.c")

    if args.compiler.endswith(".py"):
        compile_bad = run(compiler_cmd(args.compiler, bad_c))
        if compile_bad.returncode == 0:
            print("[FAIL] bad_uninitialized.c should fail semantic analysis")
            return 1
        if "uninitialized variable" not in compile_bad.stderr:
            print("[FAIL] unexpected error message for bad_uninitialized.c")
            print(compile_bad.stderr)
            return 1
        print("[PASS] semantic check for uninitialized variable")
    else:
        print("[SKIP] semantic negative test for binary compiler mode")

    if args.clang:
        out_bin = workdir / "test_example_bin"
        link = run([args.clang, str(out_ll), "-o", str(out_bin)])
        if link.returncode != 0:
            print("[FAIL] clang failed to build generated IR")
            print(link.stderr)
            return 1
        exe = run([str(out_bin)])
        if exe.returncode != 31:
            print(f"[FAIL] executable returned {exe.returncode}, expected 31")
            return 1
        print("[PASS] execute generated binary")
    else:
        print("[SKIP] clang not found")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
