# AArch64 Global Int Pointer Roundtrip Plan

## Status

Complete on 2026-03-29.

## Relationship To Roadmap

Follows:

- `ideas/closed/13_backend_aarch64_global_int_pointer_diff_plan.md`
- `ideas/open/__backend_port_plan.md`

## Goal

Promote the next bounded AArch64 global-addressing seam after `global_int_pointer_diff`: one `ptrtoint` and `inttoptr` round-trip over a defined global `int` object through the asm path.

## Why This Is Separate

- the completed `global_int_pointer_diff` slice proved bounded global-address subtraction plus one explicit element-width scaling step
- `global_int_pointer_roundtrip` is adjacent but distinct because it requires preserving a global address through integer conversion and recovery before the final load
- keeping this seam separate preserves the narrow-slice rule and avoids silently bundling generic pointer round-tripping or broad address arithmetic into one runbook

## Candidate Case

- synthetic backend: `make_global_int_pointer_roundtrip_module()`
- runtime: `tests/c/internal/backend_case/global_int_pointer_roundtrip.c`

## Validation Target

- tighten `tests/backend/backend_lir_adapter_tests.cpp` so the `global_int_pointer_roundtrip` slice stops accepting LLVM IR fallback
- promote `tests/c/internal/backend_case/global_int_pointer_roundtrip.c` through `BACKEND_OUTPUT_KIND=asm` once the bounded backend-owned path is explicit and test-backed

## Non-Goals

- new pointer-difference work
- general `ptrtoint` or `inttoptr` support outside this exact global-address round-trip seam
- aggregate pointer arithmetic
- mixed local/global address lowering in one patch
- built-in assembler or linker expansion

## Execution Notes

- prefer a parser/emitter seam that recognizes one defined global `i32`, one `ptrtoint`, one `inttoptr`, one load, and one return
- compare the runtime-emitted LIR against the synthetic module builder before changing the emitter so the bounded contract is explicit
- stop and split again if the implementation starts requiring generic integer-address materialization or non-global pointer recovery

## Completion Notes

- tightened `tests/backend/backend_lir_adapter_tests.cpp` so the synthetic round-trip fixture now matches the runtime-emitted shape with two entry allocas, one spilled integer address, one spilled recovered pointer, and one final scalar load
- added a bounded AArch64 matcher in `src/backend/aarch64/codegen/emit.cpp` that recognizes only this exact global `i32` pointer-roundtrip slice and lowers it through the backend-owned asm path
- promoted `tests/c/internal/backend_case/global_int_pointer_roundtrip.c` to `BACKEND_OUTPUT_KIND=asm` in `tests/c/internal/InternalTests.cmake`, and the runtime case now passes without LLVM IR fallback

## Leftover Issues

- general `ptrtoint` and `inttoptr` lowering outside this exact global-address round-trip seam remains out of scope for this closed slice
