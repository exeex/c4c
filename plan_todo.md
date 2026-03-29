# Struct Return Function-Pointer IR Validation Todo

Status: Active
Source Idea: ideas/open/25_frontend_struct_return_function_pointer_ir_plan.md
Source Plan: plan.md

## Active Item

- [ ] Step 1: rebuild the reproducer for `tests/c/external/gcc_torture/src/struct-ret-1.c`, capture `build/c4cll` versus Clang LLVM IR, and record the exact indirect-call callable-type/byval mismatch

## Planned Queue

- [ ] Step 2: add the narrowest failing validation around the indirect struct-return function-pointer call shape
- [ ] Step 3: fix the indirect-call lowering seam without widening into general struct ABI cleanup
- [ ] Step 4: rerun targeted validation plus full-suite regression checks and decide whether the idea is ready to close

## Completed

- [x] Activated `ideas/open/25_frontend_struct_return_function_pointer_ir_plan.md` into the active runbook

## Notes

- This idea was surfaced during clean rebuild validation while closing the x86 regalloc/peephole plan and remains intentionally separate from that backend slice.
- The known bad shape is an indirect call whose callable type no longer matches the byval pointer-style argument ABI used by the accepted direct call.
- Keep the fix local to the indirect struct-return call path unless the reproducer proves the bug lives in a shared ABI helper.

## Next Intended Slice

Capture the exact emitted indirect-call LLVM IR and the corresponding Clang IR, then turn the mismatch into one focused failing test before implementation.
