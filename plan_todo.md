# x86 Local Memory Addressing Todo

Status: Active
Source Idea: ideas/open/17_backend_x86_local_memory_addressing_plan.md
Source Plan: plan.md

## Active Item

- [ ] Step 1: reproduce the x86 asm-path failure for `tests/c/internal/backend_case/local_array.c`, capture the missing stack-base/indexed-address seam, and record the narrowest backend test surface to use next

## Planned Queue

- [ ] Step 2: add one focused backend validation around local stack-slot addressing before implementation
- [ ] Step 3: implement the smallest x86 stack-base plus indexed-address lowering needed for `local_array.c`
- [ ] Step 4: rerun targeted validation plus full-suite regression checks and decide whether this idea is ready to close

## Completed

- [x] Activated `ideas/open/17_backend_x86_local_memory_addressing_plan.md` into the active runbook

## Notes

- This plan follows the closed struct-return function-pointer IR slice and intentionally returns to the outstanding x86 backend runtime failures already present in the regression logs.
- `local_array.c` is the first bounded target because it exercises stack-local address formation without widening into global-address materialization.

## Next Intended Slice

Capture the exact current failure mode for `local_array.c`, then convert that seam into one focused failing backend test before touching x86 lowering.
