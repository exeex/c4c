# x86 Global Int Pointer Diff Plan

Status: Complete

## Relationship To Roadmap

Follows:

- `ideas/open/18_backend_x86_global_addressing_plan.md`
- `ideas/open/__backend_port_plan.md`

## Goal

Promote one bounded x86 global-addressing seam for subtraction over a global `int` array pointer path through the asm backend, including the explicit element-width scaling step.

## Why This Is Separate

- `int*` subtraction is not the same seam as `char*` subtraction because it introduces element-width scaling
- keeping it separate preserves the bounded-slice rule and gives one clear place to prove the x86 scaling contract

## Candidate Case

- `tests/c/internal/backend_case/global_int_pointer_diff.c`

## Validation Target

- tighten backend adapter tests so the `global_int_pointer_diff` seam stops accepting fallback
- promote the runtime case through `BACKEND_OUTPUT_KIND=asm`

## Non-Goals

- generic pointer arithmetic support
- pointer round-tripping
- mixed local/global address lowering
- built-in assembler or linker work

## Execution Notes

- prefer a matcher/emitter seam that recognizes one bounded global `int` array plus one subtraction and divide-by-element-width contract
- split again if the work expands into broad address arithmetic

## Completion Notes

- Added bounded x86 adapter coverage for `global_int_pointer_diff` so fallback to LLVM text is no longer accepted for this slice.
- Promoted the x86 backend path for one global `int*` subtraction seam with an explicit divide-by-four scaling step.
- Verified the targeted adapter and runtime backend tests pass, and the full-suite regression guard improved from 2318 to 2319 passing tests with no new failures.

## Leftover Issues

- None for this bounded seam. Adjacent pointer-arithmetic work remains out of scope and should be tracked as separate ideas if needed.
