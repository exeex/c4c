# x86 Global Int Pointer Roundtrip Plan

## Relationship To Roadmap

Follows:

- `ideas/open/21_backend_x86_global_int_pointer_diff_plan.md`
- `ideas/open/__backend_port_plan.md`

## Goal

Promote the next bounded x86 global-addressing seam after `global_int_pointer_diff`: one `ptrtoint` and `inttoptr` round-trip over a defined global `int` object through the asm path.

## Why This Is Separate

- this seam is adjacent to pointer-difference work but distinct because it must preserve a global address through integer conversion and recovery before the final load
- keeping it separate preserves the narrow-slice rule and avoids silently bundling generic pointer round-tripping into one runbook

## Candidate Case

- synthetic backend global-int-pointer-roundtrip fixture
- `tests/c/internal/backend_case/global_int_pointer_roundtrip.c`

## Validation Target

- tighten backend adapter tests so the round-trip slice stops accepting LLVM fallback
- promote the runtime case through `BACKEND_OUTPUT_KIND=asm` once the bounded backend-owned path is explicit

## Non-Goals

- new pointer-difference work
- general `ptrtoint` or `inttoptr` support outside this exact global-address round-trip seam
- aggregate pointer arithmetic
- built-in assembler or linker expansion

## Execution Notes

- prefer a parser/emitter seam that recognizes one defined global `i32`, one `ptrtoint`, one `inttoptr`, one load, and one return
- stop and split again if the implementation starts requiring generic integer-address materialization or non-global pointer recovery
