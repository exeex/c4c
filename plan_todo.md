# AMD64 Linux Recovery TODO

Status: Active
Source Idea: ideas/open/x86_64_linux_failures_plan.md
Source Plan: plan.md
Last Updated: 2026-03-29

## Checklist
- [x] 1. Baseline & Instrumentation (ctest subset + artifact capture)
- [x] 2. SysV Varargs classification & homes
- [x] 3. Variadic function pointer calls
- [ ] 4. Inline diagnostics shim stability
- [ ] 5. Inline asm `yield` alias
- [x] 6. Glibc macro parsing support
- [ ] 7. Validation & regression guard

## Active Slice
- Target: Step 4 Inline diagnostics shim stability
- Narrow scope: dig back into the inline diagnostics trampoline for AMD64/Linux — re-run the focused inline diagnostics cases, audit the shim’s callee-save/stack-alignment handling, and nail down a minimal repro we can iterate on without breaking the newly-stable vararg path.
- Owner notes: carry forward the Step 3 targeted logs plus the vararg regressions as guardrails while working on the shim.

## Next Slice
- Once the shim survives the focused stress test, switch to Step 5 by adding the inline asm `yield` alias coverage and then start lining up the full-suite rerun for Step 7.

## Progress Summary (2026-03-29)
- Step 1 artifacts are checked in under `ref/amd64_varargs/`, so the amd64 baseline plus clang/c4cll comparisons exist for every failure bucket we care about right now.
- Step 2 is now closed: AMD64 SysV vararg classification honors register availability for both GP and SSE slots, `c_testsuite_00204` matches clang exactly, and the refreshed targeted run (`ref/amd64_varargs/ctest_subset_20260329_step2_after_vararg_fix.log`) shows only the glibc parser failure in 00174. All GCC torture buckets in scope plus `positive_sema_*` stay green.
- Step 3 is wrapped: variadic function pointer calls now reuse the AMD64 classifier, `tests/c/internal/positive_case/ok_fn_returns_variadic_fn_ptr.c` exercises mixed GP/SSE payloads, and the focused subset log `ref/amd64_varargs/ctest_subset_20260329_step3_after_ptr_fix.log` documents the passing run that unblocked the remaining inline work.
- Step 6 is complete: the parser now treats `_Float{16,32,64,128}` and their `x` suffixed variants as builtin FP types, `c_testsuite_00174` passes again, and the reduced macro repro (`tests/c/preprocessor/glibc_mathcalls.c`) is wired into ctest via `preprocessor_glibc_mathcalls`.
- Step 7 (full-suite validation + regression guard) stays blocked on Steps 3–5 finishing; `test_before_20260329.log` is the comparison point for the future `test_after` log once the plan wraps.

## Blockers
- None — shim stabilization (Step 4) is ready to pick up now that the pointer-call regressions and glibc fallout are closed.

## Baseline Findings (2026-03-29)
- Full `ctest -j` baseline logged in `ref/amd64_varargs/test_before_20260329.log` (97% pass, 75 failures / 2248 tests); most recent targeted subset log is `ref/amd64_varargs/ctest_subset_20260329_step2.log` (37 passes / 47 tests after wiring AMD64 ABI tests, 10 fails covering `scal_to_vec*`, `stdarg_{1-3}`, `va_arg_{5,6,22}`, `c_testsuite_{00174,00204}`).
- Host triple confirmed via `clang -dumpmachine`: `x86_64-pc-linux-gnu`.
- Failure families: inline diagnostics shim (segfault), variadic fn pointer thunk (segfault), glibc macro parser (`mathcalls-helper-functions.h` parse errors), c_testsuite `00204` runtime mismatch, GCC torture varargs buckets (`va_arg_*`, `stdarg_*`, `strct_stdarg_*`, `scal_to_vec*`) all diverging from clang at runtime.
- `ref/amd64_varargs/` now holds per-bucket `c4cll.hir`, `c4cll.ll`, and `clang.ll` snapshots plus README documenting commands; `c_testsuite_00174` captures diagnostics instead of IR because preprocessing fails before HIR. `ref/amd64_varargs/ctest_subset_20260329_step2_after_vararg_fix.log` documents the latest targeted subset with only 00174 failing.

## Handoff Notes
- Step 2 work should rely on the captured bucket artifacts to design AMD64 SysV vararg classing; start with `gcc_va_arg` vs `gcc_stdarg` deltas noted above, then fold in the new ABI tests and helper routines (`classify_amd64_vararg`, `prepare_amd64_variadic_aggregate_arg`) when chasing the remaining SSE/HFA gaps.
- Keep future `test_after.log` alongside `test_before_20260329.log` once regressions improve.
