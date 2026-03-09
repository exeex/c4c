# Agent Prompt: tiny-c2ll Stabilization Playbook

Last updated: 2026-03-09

## Mission

Improve correctness of the frontend/codegen pipeline (`src/frontend/`, `src/codegen/llvm/`) with **small, verified, reversible** fixes.
Primary acceptance metric is full-suite pass rate from `ctest -j`.

## project arch

compiler pipeline:
1. src/frontend/preprocessor : source code -> text stream (not done yet, now we use clang -E instead)
2. src/frontend/lexer : text stream -> token stream
3. src/frontend/parser : token stream -> AST
4. src/frontend/sema : AST -> hir
5. src/codegen/llvm : hir -> llvm ir (.ll)

## Non-Negotiable Working Style

1. Never guess ABI behavior when a failing case can be compared against clang IR.
2. Fix one root cause at a time; prove with targeted test, then broader regression check.
3. Do not hide failures by editing vendored tests under `tests/c-testsuite/` or `tests/llvm-test-suite/`.
4. If a patch breaks core functionality tests (especially `c_testsuite` cases), inspect and compare corresponding logic in `ref/claudes-c-compiler/src/frontend/` before further patching; prefer adopting a proven handling pattern that avoids fixing A while breaking B.


## Required Debug Flow (Frontend/Backend Failures)

Given a failing case `<case>.c`:

0. Run full test logging and select a issue:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_LLVM_GCC_C_TORTURE_TESTS=ON
cmake --build build -j8
ctest --test-dir build -j --output-on-failure > test_fail_before.log
```
if `known_issues.md` exists, fix the issues first, else pick a fail case in `test_fail_before.log`

1. Reproduce with ctest:
   - `ctest --test-dir build --output-on-failure -R <test_name>`

   Test timeout policy:
   - Default per-test timeout: `30s`
   - Any test runtime `>30s` is treated as suspicious regression unless explicitly documented.

2. Capture our IR and classify the failure type to determine which file to patch:
   - `FRONTEND_FAIL`         → `src/frontend/parser/` or `src/frontend/sema/ast_to_hir.cpp`
   - `BACKEND_FAIL` / `RUNTIME_MISMATCH` → `src/codegen/llvm/hir_to_llvm.cpp`
   - gen hir: `build/tiny-c2ll-next --dump-hir /path/to/case.c`
   - gen llvm ir: `build/tiny-c2ll-next /path/to/case.c`
3. Capture clang IR on same target triple:
   - `clang -S -emit-llvm -O0 <case>.c -o /tmp/clang.ll`
   - (Use host triple by default; only add `-target aarch64-unknown-linux-gnu` if the test is explicitly arm64-specific)
4. Diff only failing function/path:
   - call signature (especially variadic)
   - aggregate coercion
   - va_list layout/offset math
   - load/store alignment and memcpy size
5. Patch smallest mismatch.
6. Re-run:
   - failing test
   - nearby bucket tests (same subsystem)
   - full `ctest -j` before handoff
```bash
rm -rf build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_LLVM_GCC_C_TORTURE_TESTS=ON
cmake --build build -j8
ctest --test-dir build -j --output-on-failure > test_fail_after.log
```
7. make sure everything good

check diffirence of `test_fail_before.log` and `test_fail_after.log`.

Full-suite result is strictly monotonic: total passed tests must increase, and no previously passing test may flip to failing.

8. (optional) Log any known-but-deferred failures in `known_issues.md` (create the file if it doesn't exist).
   Delete entries that are now resolved.

9. git commit — one commit per root cause; do not bundle unrelated fixes into a single commit.