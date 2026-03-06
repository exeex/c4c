# tiny-c2ll Plan (Frontend-Failure Focus)

Last updated: 2026-03-06

## Current State

- Active frontend is C++ in `src/frontend_c/`.
- Python frontend is removed.
- Core smoke suites pass:
  - `tiny_c2ll_tests`
  - `frontend_cxx_preprocessor_tests`
- `20010605-2.c` frontend fix is completed (`__real__/__imag__` support).

## Allowlist Strategy

- `tests/llvm_gcc_c_torture_allowlist.txt` is generated from:
  - `tests/llvm_gcc_c_torture_frontend_failures.tsv`
- Purpose: focus Claude/Codex loop on known frontend failure set instead of full ~1.7k cases.

## Current First Failure (Focused Run)

Command used:

```bash
PRUNE_FAILED_ALLOWLIST=0 ./scripts/check_progress_llvm_gcc_c_torture.sh
```

Result:
- First fail: `llvm_gcc_c_torture_ieee_pr72824_2_c`
- Failure kind: `[BACKEND_FAIL]`
- Error: invalid LLVM IR cast (`ptrtoint ptr ... to [4 x float]`)
- Classification: frontend IR generation gap (GNU vector extension semantics)

Focused-run snapshot (2026-03-06):
- 46 tests passed before first failure
- `98%` passed in the truncated first-fail run (`1/47` failed)

Recent completed slices now reflected in working tree:
- parser: `typeof(type|identifier)`, `__builtin_types_compatible_p`, `vector_size(...)` parsing hooks
- preprocessor: literal-prefix-safe macro expansion (`L/u/U/u8`), integer limit predefined macros
- IR builder: more builtins (`ffs/clz/ctz/popcount/parity`, `copysign`, `nan`, `is*`), complex compound assignment/conjugate, safer `void*` GEP element typing
- allowlist: commented known non-frontend blockers (`bcp-1.c`, `comp-goto-1.c`)

Latest slice (2026-03-06, follow-up handoff):
- parser: added `NK_OFFSETOF` support path in `parse_primary()` for `__builtin_offsetof(type, member[.submember])`
- parser: wired struct definition map updates so parse-time constant evaluation can resolve struct field offsets
- note: focused allowlist is kept in the frontend-failure list form (not pruned-to-only-last-fail form)

Action policy:
1. Skip/comment non-frontend blocker cases in allowlist.
2. Continue first-fail loop until next actionable frontend failure.
3. For `ieee/pr72824-2.c`, either:
   - implement real vector type lowering (preferred), or
   - temporarily move case out of allowlist if treating vector extension as out-of-scope.

## Active Repair Workflow

1. Run first-fail loop:
   - `PRUNE_FAILED_ALLOWLIST=0 ./scripts/check_progress_llvm_gcc_c_torture.sh`
2. If fail is non-frontend (`CLANG_COMPILE_FAIL`, platform-link `BACKEND_FAIL`):
   - comment out case in `tests/llvm_gcc_c_torture_allowlist.txt`
   - rerun.
3. If fail is frontend (`FRONTEND_FAIL`, `FRONTEND_TIMEOUT`):
   - fix smallest root cause in parser/typing/IR.
4. Commit one small slice each iteration.

## Execution Rules

1. One smallest fix slice per commit.
2. Re-run first-fail loop after each slice.
3. Do not edit vendored test-suite sources:
   - `tests/c-testsuite/`
   - `tests/llvm-test-suite/`
4. If blocked >15 min, log blocker in `build/agent_state/hard_bugs.md` and switch hypothesis.

## Useful Commands

```bash
cmake -S . -B build_debug
cmake --build build_debug -j8

PRUNE_FAILED_ALLOWLIST=0 ./scripts/check_progress_llvm_gcc_c_torture.sh

ctest --test-dir build_debug --output-on-failure -R '^llvm_gcc_c_torture_20010122_1_c$' -j 1
```
