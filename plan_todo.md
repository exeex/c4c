# AMD64 Linux Recovery TODO

Status: Active
Source Idea: ideas/open/x86_64_linux_failures_plan.md
Source Plan: plan.md
Last Updated: 2026-03-29

## Checklist
- [x] 1. Baseline & Instrumentation (ctest subset + artifact capture)
- [x] 2. SysV Varargs classification & homes
- [ ] 3. Variadic function pointer calls
- [ ] 4. Inline diagnostics shim stability
- [ ] 5. Inline asm `yield` alias
- [ ] 6. Glibc macro parsing support
- [ ] 7. Validation & regression guard

## Active Slice
- Target: Step 6 Glibc macro parsing support
- Narrow scope: unblock the glibc `mathcalls-helper-functions.h` headers that still break `c_testsuite_00174`—with Step 2 closed, the focused ctest subset (`ref/amd64_varargs/ctest_subset_20260329_step2_after_vararg_fix.log`) shows the only remaining failure is the parser choking on those nested helper macros. Time to prototype a preprocessor fix or targeted macro shim so `__value`/`__x` parameters no longer trigger the RPAREN diagnostics.
- Owner notes: Step 2 artifacts stay relevant for sanity (we now have a clean runtime diff for `c_testsuite_00204`), so keep referring to `ref/amd64_varargs/c_testsuite_00204` when validating that glibc macro work doesn’t regress AMD64 lowering.

## Next Slice
- Draft a reduced repro for the glibc helper macros (lift the problematic `bits/mathcalls-helper-functions.h` chunk into `tests/c/preprocessor/glibc_mathcalls.c`) and experiment with either parser tweaks or predefined helpers so the nested parameter macros parse cleanly before re-running the targeted ctest subset.

## Progress Summary (2026-03-29)
- Step 1 artifacts are checked in under `ref/amd64_varargs/`, so the amd64 baseline plus clang/c4cll comparisons exist for every failure bucket we care about right now.
- Step 2 is now closed: AMD64 SysV vararg classification honors register availability for both GP and SSE slots, `c_testsuite_00204` matches clang exactly, and the refreshed targeted run (`ref/amd64_varargs/ctest_subset_20260329_step2_after_vararg_fix.log`) shows only the glibc parser failure in 00174. All GCC torture buckets in scope plus `positive_sema_*` stay green.
- Steps 3–6 are untouched: once the SysV save-area work is green, the next slices are variadic function pointer calls, inline diagnostics shim fixes, asm `yield` aliasing, and the glibc macro parser gaps.
- Step 7 (full-suite validation + regression guard) stays blocked on Steps 3–6 finishing; `test_before_20260329.log` is the comparison point for the future `test_after` log once the plan wraps.

## Blockers
- None besides the known glibc macro gaps (tracked as Step 6).

## Baseline Findings (2026-03-29)
- Full `ctest -j` baseline logged in `ref/amd64_varargs/test_before_20260329.log` (97% pass, 75 failures / 2248 tests); most recent targeted subset log is `ref/amd64_varargs/ctest_subset_20260329_step2.log` (37 passes / 47 tests after wiring AMD64 ABI tests, 10 fails covering `scal_to_vec*`, `stdarg_{1-3}`, `va_arg_{5,6,22}`, `c_testsuite_{00174,00204}`).
- Host triple confirmed via `clang -dumpmachine`: `x86_64-pc-linux-gnu`.
- Failure families: inline diagnostics shim (segfault), variadic fn pointer thunk (segfault), glibc macro parser (`mathcalls-helper-functions.h` parse errors), c_testsuite `00204` runtime mismatch, GCC torture varargs buckets (`va_arg_*`, `stdarg_*`, `strct_stdarg_*`, `scal_to_vec*`) all diverging from clang at runtime.
- `ref/amd64_varargs/` now holds per-bucket `c4cll.hir`, `c4cll.ll`, and `clang.ll` snapshots plus README documenting commands; `c_testsuite_00174` captures diagnostics instead of IR because preprocessing fails before HIR. `ref/amd64_varargs/ctest_subset_20260329_step2_after_vararg_fix.log` documents the latest targeted subset with only 00174 failing.

## Handoff Notes
- Step 2 work should rely on the captured bucket artifacts to design AMD64 SysV vararg classing; start with `gcc_va_arg` vs `gcc_stdarg` deltas noted above, then fold in the new ABI tests and helper routines (`classify_amd64_vararg`, `prepare_amd64_variadic_aggregate_arg`) when chasing the remaining SSE/HFA gaps.
- Keep future `test_after.log` alongside `test_before_20260329.log` once regressions improve.
