# x86 Extern Global Array Addressing Todo

Status: Active
Source Idea: ideas/open/19_backend_x86_extern_global_array_addressing_plan.md
Source Plan: plan.md

## Active Item

- [ ] Step 1: reproduce the x86 asm-path failure for `tests/c/internal/backend_case/extern_global_array.c`, capture the missing RIP-relative base-address plus indexed-load seam, and record the narrowest backend test surface to use next

## Planned Queue

- [ ] Step 2: add one focused backend validation around x86 extern global array addressing before implementation
- [ ] Step 3: implement the smallest x86 RIP-relative base plus indexed-load lowering needed for `extern_global_array.c`
- [ ] Step 4: rerun targeted validation plus full-suite regression checks and decide whether this idea is ready to close

## Completed

- [x] Activated `ideas/open/19_backend_x86_extern_global_array_addressing_plan.md` into the active runbook

## Notes

- This plan follows the closed local-memory addressing slice and moves to the next bounded x86 global-addressing seam already represented in the open idea inventory.
- `extern_global_array.c` is the first bounded target because it exercises global base-address formation and indexed element access without yet widening into string-pool or pointer-difference behavior.

## Next Intended Slice

Capture the exact current failure mode for `extern_global_array.c`, then convert that seam into one focused failing backend test before touching x86 lowering.
