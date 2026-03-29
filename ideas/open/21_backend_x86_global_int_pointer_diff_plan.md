# x86 Global Int Pointer Diff Plan

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
