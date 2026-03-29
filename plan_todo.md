# AMD64 Linux Recovery TODO

Status: Active
Source Idea: ideas/open/x86_64_linux_failures_plan.md
Source Plan: plan.md
Last Updated: 2026-03-29

## Checklist
- [x] 1. Baseline & Instrumentation (ctest subset + artifact capture)
- [x] 2. SysV Varargs classification & homes
- [x] 3. Variadic function pointer calls
- [x] 4. Inline diagnostics shim stability
- [x] 5. Inline asm `yield` alias
- [x] 6. Glibc macro parsing support
- [ ] 7. Validation & regression guard

## Active Slice
- Target: Step 7 Validation & regression guard
- Narrow scope: pave the `test_after` snapshot by re-running the full suite on the latest AMD64 fixes, diff against `ref/amd64_varargs/test_before_20260329.log`, and stash both the log + summary deltas so we can decide whether to close or split remaining failures.
- Owner notes: kick off the `ctest -j` sweep immediately after refreshing the varargs subset so there’s no drift between the focused guardrail and the closing log pair.

## Next Slice
- If any stragglers remain after the full-suite pass, triage them into either follow-on Step 7 notes or a fresh idea file so this plan can be closed cleanly.

## Progress Summary (2026-03-29)
- Step 1 artifacts are checked in under `ref/amd64_varargs/`, so the amd64 baseline plus clang/c4cll comparisons exist for every failure bucket we care about right now.
- Step 2 is now closed: AMD64 SysV vararg classification honors register availability for both GP and SSE slots, `c_testsuite_00204` matches clang exactly, and the refreshed targeted run (`ref/amd64_varargs/ctest_subset_20260329_step2_after_vararg_fix.log`) shows only the glibc parser failure in 00174. All GCC torture buckets in scope plus `positive_sema_*` stay green.
- Step 3 is wrapped: variadic function pointer calls now reuse the AMD64 classifier, `tests/c/internal/positive_case/ok_fn_returns_variadic_fn_ptr.c` exercises mixed GP/SSE payloads, and the focused subset log `ref/amd64_varargs/ctest_subset_20260329_step3_after_ptr_fix.log` documents the passing run that unblocked the remaining inline work.
- Step 4 is buttoned up: added `tests/c/internal/positive_case/inline_diagnostics_runtime.c` to hammer the recursive + variadic `always_inline` fallback path in tight loops, reconfigured CTest so it runs as `positive_sema_inline_diagnostics_runtime_c`, and verified both inline diagnostics cases are green via `ctest --test-dir build -R inline_diagnostics -j4 --output-on-failure`.
- Step 6 is complete: the parser now treats `_Float{16,32,64,128}` and their `x` suffixed variants as builtin FP types, `c_testsuite_00174` passes again, and the reduced macro repro (`tests/c/preprocessor/glibc_mathcalls.c`) is wired into ctest via `preprocessor_glibc_mathcalls`.
- Step 5 is closed: inline asm lowering now rewrites `yield` → `pause` on AMD64, `positive_sema_linux_stage2_repro_03_asm_volatile_c` compiles/run under both clang + c4cll, and the focused log `ref/amd64_varargs/ctest_subset_20260329_step5_yield_alias.log` proves the alias alongside the inline diagnostics stressors.
- Step 7 (full-suite validation + regression guard) now unblocks — `test_before_20260329.log` remains the baseline until we record the corresponding `test_after` snapshot on the refreshed compiler.

## Blockers
- None — Step 7 can proceed immediately after capturing the new `test_after` log; if fresh failures surface they should turn into a follow-on idea rather than mutating this runbook.

## Baseline Findings (2026-03-29)
- Full `ctest -j` baseline logged in `ref/amd64_varargs/test_before_20260329.log` (97% pass, 75 failures / 2248 tests); most recent targeted subset log is `ref/amd64_varargs/ctest_subset_20260329_step2.log` (37 passes / 47 tests after wiring AMD64 ABI tests, 10 fails covering `scal_to_vec*`, `stdarg_{1-3}`, `va_arg_{5,6,22}`, `c_testsuite_{00174,00204}`).
- Host triple confirmed via `clang -dumpmachine`: `x86_64-pc-linux-gnu`.
- Failure families: inline diagnostics shim (segfault), variadic fn pointer thunk (segfault), glibc macro parser (`mathcalls-helper-functions.h` parse errors), c_testsuite `00204` runtime mismatch, GCC torture varargs buckets (`va_arg_*`, `stdarg_*`, `strct_stdarg_*`, `scal_to_vec*`) all diverging from clang at runtime.
- `ref/amd64_varargs/` now holds per-bucket `c4cll.hir`, `c4cll.ll`, and `clang.ll` snapshots plus README documenting commands; `c_testsuite_00174` captures diagnostics instead of IR because preprocessing fails before HIR. `ref/amd64_varargs/ctest_subset_20260329_step2_after_vararg_fix.log` documents the latest targeted subset with only 00174 failing.

## Handoff Notes
- Step 2 work should rely on the captured bucket artifacts to design AMD64 SysV vararg classing; start with `gcc_va_arg` vs `gcc_stdarg` deltas noted above, then fold in the new ABI tests and helper routines (`classify_amd64_vararg`, `prepare_amd64_variadic_aggregate_arg`) when chasing the remaining SSE/HFA gaps.
- Keep future `test_after.log` alongside `test_before_20260329.log` once regressions improve.
