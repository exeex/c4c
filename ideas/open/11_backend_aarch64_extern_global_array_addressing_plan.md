# AArch64 Extern Global Array Addressing Plan

## Relationship To Roadmap

Follows:

- `ideas/closed/10_backend_aarch64_global_addressing_plan.md`
- `ideas/open/__backend_port_plan.md`

## Goal

Promote the next bounded AArch64 global-addressing seam after `string_literal_char`: one extern global array base-address plus indexed element load through the asm path.

## Why This Is Separate

- the completed `string_literal_char` slice proved one explicit string-pool base-address formation seam without widening into general pointer arithmetic
- `extern_global_array` is adjacent but distinct: it adds global-symbol decay and indexed element addressing for a non-string object
- keeping this as a separate idea preserves the narrowness rule from the prior runbook and avoids silently bundling broader pointer/address work

## Candidate Case

- synthetic backend: `make_extern_global_array_load_module()`

## Validation Target

- tighten `tests/backend/backend_lir_adapter_tests.cpp` so the extern-array slice stops accepting LLVM IR fallback
- promote a matching bounded runtime case through `BACKEND_OUTPUT_KIND=asm` only if a minimal frontend-emitted candidate exists without pulling in broad pointer generalization

## Non-Goals

- general aggregate pointer arithmetic
- mixed local/global address lowering in one patch
- pointer-difference and round-trip work such as `global_char_pointer_diff`, `global_int_pointer_diff`, or `global_int_pointer_roundtrip`
