# Built-in x86 Linker Todo

Status: Active
Source Idea: ideas/open/24_backend_builtin_linker_x86_plan.md
Source Plan: plan.md

## Current Active Item

- [ ] Step 1: Inspect the first bounded x86 linker slice
- Iteration slice: inventory the existing x86 linker tests, fixtures, and
  backend-owned object paths to choose the first relocation-bearing or
  multi-object executable case for built-in linking.

## Planned Items

- [ ] Step 1: Inspect the first bounded x86 linker slice
- [ ] Step 2: Compile-integrate the minimum x86 linker orchestration surface
- [ ] Step 3: Port the bounded shared input and symbol-loading path
- [ ] Step 4: Implement the minimum x86 relocation and resolution slice
- [ ] Step 5: Emit one bounded static x86 executable
- [ ] Step 6: Validate against the external linker path

## Completed Items

- [x] Activated `ideas/open/24_backend_builtin_linker_x86_plan.md` into the active runbook

## Next Intended Slice

- Inspect the current x86 linker and shared-linker tests before editing code.
- Choose one narrow static executable bring-up case that depends on the newly
  closed built-in x86 assembler slice.
- Keep the first linker slice focused on static executable output only.

## Blockers

- None recorded.

## Resume Notes

- The preceding assembler runbook is closed at
  `ideas/closed/23_backend_builtin_assembler_x86_plan.md`.
- Keep this linker runbook scoped to x86 static linking; do not absorb dynamic
  linking or broad relocation work unless the first slice strictly requires it.
- Prefer a mechanical port of the ref/shared linker structure over an early
  redesign of linker abstractions.
