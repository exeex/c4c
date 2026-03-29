# AMD64 Linux Recovery TODO

Status: Active
Source Idea: ideas/open/x86_64_linux_failures_plan.md
Source Plan: plan.md
Last Updated: 2026-03-29

## Checklist
- [x] 1. Baseline & Instrumentation (ctest subset + artifact capture)
- [ ] 2. SysV Varargs classification & homes
- [ ] 3. Variadic function pointer calls
- [ ] 4. Inline diagnostics shim stability
- [ ] 5. Inline asm `yield` alias
- [ ] 6. Glibc macro parsing support
- [ ] 7. Validation & regression guard

## Active Slice
- Target: Step 2 SysV Varargs classification & homes
- Narrow scope: keep pushing AMD64 SysV vararg lowering – vector-register spill alignment is now fixed via the stack-allocation alignment bump, so `scal_to_vec{1,2}` are green; the remaining Step 2 work is the stubborn `c_testsuite_{00174,00204}` pair (00204 still trips the runtime vararg checker, 00174 is still blocked on glibc macro parsing in Step 6).
- Owner notes: new targeted logs show the vector segfaults are gone (`ref/amd64_varargs/ctest_scal_to_vec_20260329_before.log` vs `ref/amd64_varargs/ctest_scal_to_vec_20260329_after.log`). Continue using the bucket artifacts in `ref/amd64_varargs/*`—focus now shifts to capturing fresh clang vs c4cll traces for `c_testsuite_00204` once the parser half (00174) is unblocked or surfaced as a distinct Step 6 action.

## Next Slice
- Re-run the targeted subset (va_arg/stdarg/c_testsuite focus) to confirm `scal_to_vec*` stays green, then capture a narrowed repro plus clang IR/runtime trace for `c_testsuite_00204` so we can close out Step 2 once the parser-side 00174 issue is addressed in Step 6.

## Progress Summary (2026-03-29)
- Step 1 artifacts are checked in under `ref/amd64_varargs/`, so the amd64 baseline plus clang/c4cll comparisons exist for every failure bucket we care about right now.
- Step 2 work already landed the new ABI-focused internal tests and wiring inside `calling_convention.*`; the AMD64 `byval` fix knocked out `stdarg_3` and `va_arg_22`, and the latest stack-alignment tweak eliminated the vector segfaults (`scal_to_vec{1,2}` now pass per `ctest_scal_to_vec_20260329_after.log`). Remaining effort is concentrated on the glibc parser/runtime pair (`c_testsuite_{00174,00204}`) noted in `ref/amd64_varargs/ctest_subset_20260329_step2_ldfix.log`.
- Steps 3–6 are untouched: once the SysV save-area work is green, the next slices are variadic function pointer calls, inline diagnostics shim fixes, asm `yield` aliasing, and the glibc macro parser gaps.
- Step 7 (full-suite validation + regression guard) stays blocked on Steps 2–6 finishing; `test_before_20260329.log` is the comparison point for the future `test_after` log.

## Blockers
- None yet; awaiting new baseline artifacts.

## Baseline Findings (2026-03-29)
- Full `ctest -j` baseline logged in `ref/amd64_varargs/test_before_20260329.log` (97% pass, 75 failures / 2248 tests); most recent targeted subset log is `ref/amd64_varargs/ctest_subset_20260329_step2.log` (37 passes / 47 tests after wiring AMD64 ABI tests, 10 fails covering `scal_to_vec*`, `stdarg_{1-3}`, `va_arg_{5,6,22}`, `c_testsuite_{00174,00204}`).
- Host triple confirmed via `clang -dumpmachine`: `x86_64-pc-linux-gnu`.
- Failure families: inline diagnostics shim (segfault), variadic fn pointer thunk (segfault), glibc macro parser (`mathcalls-helper-functions.h` parse errors), c_testsuite `00204` runtime mismatch, GCC torture varargs buckets (`va_arg_*`, `stdarg_*`, `strct_stdarg_*`, `scal_to_vec*`) all diverging from clang at runtime.
- `ref/amd64_varargs/` now holds per-bucket `c4cll.hir`, `c4cll.ll`, and `clang.ll` snapshots plus README documenting commands; `c_testsuite_00174` captures diagnostics instead of IR because preprocessing fails before HIR.

## Handoff Notes
- Step 2 work should rely on the captured bucket artifacts to design AMD64 SysV vararg classing; start with `gcc_va_arg` vs `gcc_stdarg` deltas noted above, then fold in the new ABI tests and helper routines (`classify_amd64_vararg`, `prepare_amd64_variadic_aggregate_arg`) when chasing the remaining SSE/HFA gaps.
- Keep future `test_after.log` alongside `test_before_20260329.log` once regressions improve.
