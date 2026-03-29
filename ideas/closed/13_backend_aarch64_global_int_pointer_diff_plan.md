# AArch64 Global Int Pointer Difference Plan

Status: Complete

## Relationship To Roadmap

Follows:

- `ideas/closed/12_backend_aarch64_global_char_pointer_diff_plan.md`
- `ideas/open/__backend_port_plan.md`

## Goal

Promote the next bounded AArch64 global-addressing seam after `global_char_pointer_diff`: one scaled pointer-difference comparison over a defined global `int` array through the asm path.

## Why This Is Separate

- the completed `global_char_pointer_diff` slice proved bounded backend-owned address subtraction for one mutable global byte array without generic pointer arithmetic
- `global_int_pointer_diff` is adjacent but distinct because it adds one explicit element-width scaling step after the byte-granular subtraction
- keeping this seam separate preserves the narrow-slice rule and avoids silently bundling pointer round-tripping or general `ptrtoint`/`inttoptr` support into the same runbook

## Candidate Case

- synthetic backend: `make_global_int_pointer_diff_module()`
- runtime: `tests/c/internal/backend_case/global_int_pointer_diff.c`

## Validation Target

- tighten `tests/backend/backend_lir_adapter_tests.cpp` so the `global_int_pointer_diff` slice stops accepting LLVM IR fallback
- promote `tests/c/internal/backend_case/global_int_pointer_diff.c` through `BACKEND_OUTPUT_KIND=asm` once the bounded backend-owned path is explicit and test-backed

## Non-Goals

- pointer round-trip work such as `global_int_pointer_roundtrip`
- general aggregate pointer arithmetic
- mixed local/global address lowering in one patch
- generic `ptrtoint` or `inttoptr` support outside this exact seam
- built-in assembler or linker expansion

## Execution Notes

- prefer a parser/emitter seam that recognizes one defined global `int` array base address, one constant element offset, one byte-granular subtraction, one divide by the element width, and one equality comparison
- compare the runtime-emitted LIR against the synthetic module builder before changing the emitter so the bounded contract is explicit
- stop and split again if the implementation starts requiring pointer round-tripping, variable element widths beyond this exact `[2 x i32]` shape, or generic division lowering

## Completion Notes

- completed the bounded AArch64 asm path for `global_int_pointer_diff` with backend-owned `.bss` emission, byte subtraction, explicit scale-by-4 lowering via `lsr`, and equality comparison lowering
- aligned `make_global_int_pointer_diff_module()` with the frontend-emitted two-base-GEP runtime shape so synthetic and runtime validation hit the same bounded matcher
- tightened `tests/backend/backend_lir_adapter_tests.cpp` to reject LLVM IR fallback for this slice and enabled the runtime case through `BACKEND_OUTPUT_KIND=asm`
- verified targeted coverage with `backend_lir_adapter_tests`, `backend_runtime_global_char_pointer_diff`, and `backend_runtime_global_int_pointer_diff`
- verified the full suite remained monotonic: `test_before.log` and `test_after.log` both report `99% tests passed, 4 tests failed out of 580`

## Leftover Issues

- `global_int_pointer_roundtrip` remains a separate follow-on slice and should not be absorbed into this completed pointer-difference seam
