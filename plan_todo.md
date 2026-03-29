# Built-in x86 Linker Todo

Status: Active
Source Idea: ideas/open/24_backend_builtin_linker_x86_plan.md
Source Plan: plan.md

## Current Active Item

- [ ] Step 2: Compile-integrate the minimum x86 linker orchestration surface
- Iteration slice: carry the new x86 shared-parser-backed inspection/load seam
  into the next bounded linker-owned operation by applying the first PLT32
  relocation and emitting one static executable image for the same `main ->
  helper_ext` case.

## Planned Items

- [ ] Step 1: Inspect the first bounded x86 linker slice
- [ ] Step 2: Compile-integrate the minimum x86 linker orchestration surface
- [ ] Step 3: Port the bounded shared input and symbol-loading path
- [ ] Step 4: Implement the minimum x86 relocation and resolution slice
- [ ] Step 5: Emit one bounded static x86 executable
- [ ] Step 6: Validate against the external linker path

## Completed Items

- [x] Activated `ideas/open/24_backend_builtin_linker_x86_plan.md` into the active runbook
- [x] Step 1: identified the first bounded x86 linker slice as a two-object
  static `main -> helper_ext` case with one `.text` `R_X86_64_PLT32`
  relocation, shared-parser-loaded ELF objects/archives, and only merged
  allocatable `.text` output in the initial inspection seam
- [x] Added x86 linker inspection/load tests and a minimal compile-integrated
  x86 linker seam for shared-parser-backed object and archive loading

## Next Intended Slice

- Reuse the same two-object x86 fixture pair to add relocation-application
  tests for the first `R_X86_64_PLT32` call edge.
- Emit one bounded static x86 executable image for that slice instead of
  widening into shared-library or dynamic-linking behavior.
- Keep the x86 build integration limited to the linker units needed by the
  active slice; the mirrored broad x86 emitter surface still needs separate
  cleanup before it can be compiled wholesale under the current toolchain.

## Blockers

- None recorded.

## Resume Notes

- The preceding assembler runbook is closed at
  `ideas/closed/23_backend_builtin_assembler_x86_plan.md`.
- Keep this linker runbook scoped to x86 static linking; do not absorb dynamic
  linking or broad relocation work unless the first slice strictly requires it.
- Prefer a mechanical port of the ref/shared linker structure over an early
  redesign of linker abstractions.
- Inventory result for this iteration: the bounded first x86 slice should match
  the existing backend-owned two-object pattern already used by AArch64 tests:
  one caller object defining `main`, one helper object defining `helper_ext`,
  one `.text` relocation edge, and only merged allocatable `.text` output in
  the initial inspection seam.
- Regression check for this iteration: `test_before.log` and `test_after.log`
  both report `99% tests passed, 19 tests failed out of 2339` with no newly
  failing tests.
