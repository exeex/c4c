# x86 Global Char Pointer Diff Plan

## Relationship To Roadmap

Follows:

- `ideas/open/18_backend_x86_global_addressing_plan.md`
- `ideas/open/__backend_port_plan.md`

## Goal

Promote one bounded x86 global-addressing seam for subtraction over a global `char` pointer path through the asm backend.

## Why This Is Separate

- `char*` pointer subtraction is adjacent to global-addressing but has its own width semantics
- this seam is narrower than broad pointer arithmetic and should stay isolated
- keeping it separate avoids silently bundling array decay, string-pool work, and integer-element scaling into one runbook

## Candidate Case

- `tests/c/internal/backend_case/global_char_pointer_diff.c`

## Validation Target

- backend adapter coverage rejects LLVM fallback for this exact `char*` pointer-difference seam
- the runtime case passes through `BACKEND_OUTPUT_KIND=asm`

## Non-Goals

- integer-element pointer subtraction
- pointer round-tripping
- broad pointer arithmetic support
- mixed local/global address lowering

## Execution Notes

- keep the first slice focused on one bounded global base-address plus subtraction path
- stop and split again if the emitter starts needing generic pointer materialization helpers
