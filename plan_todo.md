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
- Narrow scope: finish AMD64 SysV vararg lowering now that `src/codegen/llvm/calling_convention.{cpp,hpp}` and `stmt_emitter` split aggregates into INTEGER/SSE chunks; chase the remaining `stdarg_*`, `va_arg_*`, and `scal_to_vec*` regressions uncovered in the latest targeted ctest run.
- Owner notes: `ref/amd64_varargs/ctest_subset_20260329_step2.log` captures the 47-test subset (10 fails) after wiring the new ABI tests in `tests/c/internal/abi/*.c`. Remaining buckets all hit HFA/SSE or glibc macro issues—use the refreshed `clang.ll` vs `c4cll.ll` pairs in `ref/amd64_varargs/*/` to see which registers still diverge. Fresh run after switching long double lowering to x86_fp80 lives in `ref/amd64_varargs/ctest_subset_20260329_step2_ldfix.log` (44-case slice, 6 fails: `scal_to_vec{1,2}`, `stdarg_3`, `va_arg_22`, and the glibc parser/runtime pair `00174`/`00204`). Focus next on the remaining HFA long double path (`va_arg_22`/`stdarg_3`) and the vector segfaults; glibc macro parsing stays for Step 6.

## Next Slice
- Reconcile the remaining HFA/SSE register homes for the stubborn GCC torture cases (`stdarg_3`, `va_arg_22`, `scal_to_vec*`) so the amd64 register save area matches clang, then re-baseline the subset before moving on to function pointer calls.

## Progress Summary (2026-03-29)
- Step 1 artifacts are checked in under `ref/amd64_varargs/`, so the amd64 baseline plus clang/c4cll comparisons exist for every failure bucket we care about right now.
- Step 2 work already landed the new ABI-focused internal tests and wiring inside `calling_convention.*`; remaining effort is focused on SSE/HFA register saves that cause the current failure set (`scal_to_vec{1,2}`, `stdarg_3`, `va_arg_22`, `c_testsuite_{00174,00204}`) per `ref/amd64_varargs/ctest_subset_20260329_step2_ldfix.log` after the x86_fp80 long-double fix.
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
