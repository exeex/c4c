# Agent Prompt: c4 Issue Fixing

Last updated: 2026-03-13

## Mission

Fix concrete compiler issues with small, verified, reversible changes.
Primary success criteria:

1. The selected failing case becomes correct.
2. No previously passing test regresses.
3. Full-suite results remain monotonic.

## Project Architecture

Compiler pipeline:

1. `src/frontend/preprocessor`: source code -> preprocessed text stream
2. `src/frontend/lexer`: text stream -> token stream
3. `src/frontend/parser`: token stream -> AST
4. `src/frontend/sema`: AST -> validated AST
5. `src/frontend/hir`: validated AST -> HIR
6. `src/codegen/llvm`: HIR -> LLVM IR (`.ll`)

## Non-Negotiable Working Style

1. Prioritize fixing a real reproduced failure over speculative cleanup.
2. Fix one root cause at a time. Do not bundle unrelated fixes together.
3. Never guess ABI behavior when a failing case can be compared against Clang IR.
4. Treat Clang/LLVM as the behavioral reference:
   - Preprocessor mode `c4cll --pp-only` should align with `clang -E`
   - Frontend/codegen mode `c4cll` should align with `clang -S -emit-llvm`
5. Do not hide failures by weakening tests unless the test itself is invalid.
6. If Clang cannot compile or run a test case, it is acceptable to mark that case as ignored, but only as a last resort and with an explicit reason.

## Required Issue Flow

Given a failing case `<case>.c`:

0. Record the current full-suite baseline:

```bash
cmake -S . -B build
cmake --build build -j8
ctest --test-dir build -j --output-on-failure > test_before.log
```

If `known_issues.md` exists, work through it first. Otherwise, pick one failing case from `test_before.log`.

1. Reproduce the failure with `ctest`:
   - `ctest --test-dir build --output-on-failure -R <test_name>`

   Timeout policy:
   - Default per-test timeout: `30s`
   - Any test runtime longer than `30s` is suspicious unless explicitly documented

2. Capture our output and classify the affected area:
   - `PREPROCESSOR` -> `src/frontend/preprocessor/`
   - `FRONTEND_FAIL` -> `src/frontend/parser/`, `src/frontend/sema/`, or `src/frontend/hir/`
   - `BACKEND_FAIL` or `RUNTIME_MISMATCH` -> `src/codegen/llvm/`
   - Generate HIR: `build/c4cll --dump-hir /path/to/case.c`
   - Generate LLVM IR: `build/c4cll /path/to/case.c`

3. Compare against the reference behavior:
   - For preprocessing, compare against `clang -E`
   - For frontend/codegen, capture Clang IR on the same target triple:
     - `clang -S -emit-llvm -O0 <case>.c -o /tmp/clang.ll`
   - Use the host triple by default
   - Only add an explicit target such as `-target aarch64-unknown-linux-gnu` when the test is target-specific

4. Diff only the relevant function or path. Focus on:
   - call signatures, especially variadic calls
   - aggregate coercion
   - `va_list` layout and offset math
   - load/store alignment and `memcpy` size

5. Patch the smallest confirmed mismatch.

6. Re-run:
   - the targeted failing test
   - nearby tests in the same subsystem
   - the full `ctest -j` suite before handoff

```bash
rm -rf build
cmake -S . -B build
cmake --build build -j8
ctest --test-dir build -j --output-on-failure > test_after.log
```

7. Verify the result:
   - compare `test_before.log` and `test_after.log`
   - full-suite results must be monotonic
   - no previously passing test may become failing
   - total passed tests in `test_after.log` must be greater than or equal to `test_before.log`

8. Optionally update `known_issues.md`:
   - add known deferred failures if needed
   - delete entries that are now resolved

9. Commit one root cause per commit.
