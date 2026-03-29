# x86 Global Char Pointer Diff Todo

Status: Active
Source Idea: ideas/open/20_backend_x86_global_char_pointer_diff_plan.md
Source Plan: plan.md

## Active Item

- [ ] Step 1: reproduce the x86 asm-path failure for `tests/c/internal/backend_case/global_char_pointer_diff.c`, capture the missing global base-address plus byte-pointer-difference seam, and record the narrowest backend test surface to use next

## Planned Queue

- [ ] Step 2: add one focused backend validation around x86 global `char*` pointer-difference lowering before implementation
- [ ] Step 3: implement the smallest x86 global base plus byte-pointer subtraction lowering needed for `global_char_pointer_diff.c`
- [ ] Step 4: rerun targeted validation plus full-suite regression checks and decide whether this idea is ready to close

## Completed

- [x] Activated `ideas/open/20_backend_x86_global_char_pointer_diff_plan.md` into the active runbook

## Notes

- This plan follows the completed extern-global-array addressing slice and moves to the next bounded x86 global-addressing seam already represented in the open idea inventory.
- `global_char_pointer_diff.c` is the next bounded target because it exercises global base-address formation and subtraction at byte granularity without yet adding integer-element scaling.

## Next Intended Slice

Capture the exact current failure mode for `global_char_pointer_diff.c`, then convert that seam into one focused failing backend test before touching x86 lowering.
