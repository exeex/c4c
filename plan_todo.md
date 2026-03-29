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
- Narrow scope: study `ref/amd64_varargs/*/` IR deltas (especially `gcc_va_arg`, `gcc_stdarg`, `gcc_scal_to_vec`) to draft AMD64 classing + va_list spill layout updates in `src/codegen/llvm/calling_convention.cpp`.
- Owner notes: use the freshly captured `clang.ll` vs `c4cll.ll` pairs plus runtime crashes in `ctest_subset_20260329.log` to map which args drop/alias.

## Next Slice
- Prototype register save-area emission + `va_start` tests after confirming classifier design differences vs clang in Step 2 analysis.

## Blockers
- None yet; awaiting new baseline artifacts.

## Baseline Findings (2026-03-29)
- Full `ctest -j` baseline logged in `ref/amd64_varargs/test_before_20260329.log` (97% pass, 75 failures / 2248 tests); targeted subset log stored as `ctest_subset_20260329.log` (34 failures / 44 tests).
- Host triple confirmed via `clang -dumpmachine`: `x86_64-pc-linux-gnu`.
- Failure families: inline diagnostics shim (segfault), variadic fn pointer thunk (segfault), glibc macro parser (`mathcalls-helper-functions.h` parse errors), c_testsuite `00204` runtime segfault, GCC torture varargs buckets (`va_arg_*` 23 cases, `stdarg_*` 4 cases, `strct_stdarg_*` 1 case, `scal_to_vec*` 2 cases) all diverging from clang at runtime.
- `ref/amd64_varargs/` now holds per-bucket `c4cll.hir`, `c4cll.ll`, and `clang.ll` snapshots plus README documenting commands; `c_testsuite_00174` captures diagnostics instead of IR because preprocessing fails before HIR.

## Handoff Notes
- Step 2 work should rely on the captured bucket artifacts to design AMD64 SysV vararg classing; start with `gcc_va_arg` vs `gcc_stdarg` deltas noted above.
- Keep future `test_after.log` alongside `test_before_20260329.log` once regressions improve.
