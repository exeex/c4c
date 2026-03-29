# x86 Global Int Pointer Roundtrip Plan

Status: Complete

## Completion Summary

- Added an x86 backend adapter contract for the bounded `global_int_pointer_roundtrip` slice so the backend no longer accepts LLVM-text fallback for this case.
- Implemented the matching x86 parser/emitter seam for one defined global `i32`, one `ptrtoint`, one `inttoptr`, one final load, and one return.
- Promoted `tests/c/internal/backend_case/global_int_pointer_roundtrip.c` through `BACKEND_OUTPUT_KIND=asm`; the runtime case now passes through the backend-owned x86 asm path.

## Validation Notes

- Targeted coverage passed for `backend_lir_adapter_tests`, `backend_runtime_global_char_pointer_diff`, `backend_runtime_global_int_pointer_diff`, and `backend_runtime_global_int_pointer_roundtrip`.
- Full-suite regression guard passed with a monotonic improvement: `2319` passed / `20` failed before, `2320` passed / `19` failed after.

## Leftover Issues

- None discovered within this bounded round-trip slice.

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
