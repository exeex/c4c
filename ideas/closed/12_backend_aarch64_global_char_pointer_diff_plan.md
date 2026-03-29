# AArch64 Global Char Pointer Difference Plan

## Status

Complete on 2026-03-29.

## Relationship To Roadmap

Follows:

- `ideas/closed/11_backend_aarch64_extern_global_array_addressing_plan.md`
- `ideas/open/__backend_port_plan.md`

## Goal

Promote the next bounded AArch64 global-addressing seam after `extern_global_array`: one byte-granular pointer-difference comparison over a defined global byte array through the asm path.

## Why This Is Separate

- the completed extern-global-array slice proved backend-owned base-address formation plus one fixed-offset load without widening into pointer arithmetic
- `global_char_pointer_diff` is the smallest adjacent case that adds explicit address subtraction while still avoiding scaled element math and pointer round-tripping
- keeping this seam separate preserves the narrow-slice rule and avoids silently bundling `sdiv`-based element scaling or `inttoptr` recovery into the same runbook

## Candidate Case

- synthetic backend: `make_global_char_pointer_diff_module()`
- runtime: `tests/c/internal/backend_case/global_char_pointer_diff.c`

## Validation Target

- tighten `tests/backend/backend_lir_adapter_tests.cpp` so the `global_char_pointer_diff` slice stops accepting LLVM IR fallback
- promote `tests/c/internal/backend_case/global_char_pointer_diff.c` through `BACKEND_OUTPUT_KIND=asm` once the bounded backend-owned path is explicit and test-backed

## Non-Goals

- scaled pointer-difference work such as `global_int_pointer_diff`
- pointer round-trip work such as `global_int_pointer_roundtrip`
- general aggregate pointer arithmetic
- mixed local/global address lowering in one patch
- built-in assembler or linker expansion

## Execution Notes

- prefer a parser/emitter seam that recognizes one defined global byte-array base-address plus one constant-byte-offset subtraction shape
- keep the first asm lowering Linux/AArch64-oriented and bounded to the existing single-function `main` shape
- stop and split again if the implementation starts requiring generic `ptrtoint` support outside this exact byte-array difference seam

## Completion Notes

- aligned the synthetic backend slice with the runtime-emitted LIR shape: two explicit global base GEPs, widened constant byte indices, `ptrtoint`, one `sub`, one `icmp eq`, and one `zext`
- tightened `tests/backend/backend_lir_adapter_tests.cpp` so `global_char_pointer_diff` now requires bounded AArch64 asm instead of LLVM IR fallback
- promoted `tests/c/internal/backend_case/global_char_pointer_diff.c` to `BACKEND_OUTPUT_KIND=asm` and verified the runtime case still returns `1`
- added a minimal AArch64 emitter slice in `src/backend/aarch64/codegen/emit.cpp` that owns only this exact byte-array `+1` pointer-difference comparison and emits mutable global storage plus bounded address subtraction
- full-suite regression guard passed with no newly failing tests: before `576/580`, after `576/580`

## Leftover Follow-On Notes

- `global_int_pointer_diff` remains separate because it introduces scaled element-width division after the byte-granular subtraction
- `global_int_pointer_roundtrip` remains separate because it requires pointer round-tripping rather than direct comparison of the subtraction result
