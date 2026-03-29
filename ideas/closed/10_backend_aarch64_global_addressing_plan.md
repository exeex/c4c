# AArch64 Global Addressing Plan

Status: Complete

## Completion Summary

- completed the first bounded backend-owned global-addressing slice by lowering `tests/c/internal/backend_case/string_literal_char.c` through the AArch64 asm path
- tightened synthetic backend coverage in `tests/backend/backend_lir_adapter_tests.cpp` around string-pool base-address formation, constant byte indexing, and the final byte-to-`i32` promotion
- added an ELF object contract for the same string-literal seam and promoted the runtime case through `BACKEND_OUTPUT_KIND=asm`
- validated monotonic regression results: `test_before.log` recorded 5 failing tests out of 577 and `test_after.log` recorded 4 failing tests out of 578

## Follow-On Work

- remaining adjacent address-formation work was intentionally split back out rather than broaden this finished slice
- see `ideas/open/11_backend_aarch64_extern_global_array_addressing_plan.md` for the next explicit global-addressing runbook candidate

## Relationship To Roadmap

Umbrella source: `ideas/open/__backend_port_plan.md`

Depends on:

- `ideas/open/02_backend_aarch64_port_plan.md`

Should follow:

- the initial hello-world-level AArch64 `.s -> .o` bring-up tracked in `ideas/open/02_backend_aarch64_port_plan.md`

## Goal

Extend the AArch64 backend beyond exact scalar global loads into explicit backend-owned lowering for global and constant address formation, starting with the smallest cases that require array decay, indexed global addressing, string-pool addressing, or pointer/address round-tripping.

## Why This Is Separate

- the current bring-up runbook already proved the narrow `.s -> .o` path for direct returns, direct calls, compare-and-branch, local single-slot rewrites, scalar global definitions, and extern scalar global loads
- the next remaining global-addressing cases are not another exact `load @symbol` seam
- backend and runtime fixtures now show the next candidates require `getelementptr`-based decay/indexing, string-pool base-address formation, pointer subtraction, or `ptrtoint`/`inttoptr` round-tripping
- widening the active bring-up runbook to absorb that work would silently expand the emitter contract from narrow symbol loads into broader pointer/address lowering

## Initial Targets

- one bounded extern-global-array element load through the AArch64 asm path
- one bounded string-pool decay/indexed-byte load through the AArch64 asm path
- one bounded global-address round-trip or pointer-difference slice only after the first address-formation seam is explicit and test-backed

## Explicit Non-Goals

- broad local-memory lowering
- general aggregate or pointer arithmetic support
- full relocation-model coverage
- built-in assembler or linker work
- regalloc or peephole cleanup beyond what the first global-addressing slice strictly requires

## Suggested Execution Order

1. capture the real frontend-emitted AArch64 LIR for the smallest remaining global-addressing runtime case and the matching synthetic backend fixture
2. define the narrowest backend-owned representation needed for global/string base-address formation plus one indexed load seam
3. add or tighten targeted adapter/emitter tests before implementation
4. route the corresponding runtime test through `BACKEND_OUTPUT_KIND=asm` only when the backend-owned path is explicit and test-backed
5. stop and split again if the first implementation attempt pulls in broad pointer arithmetic, relocation generalization, or mixed local/global address lowering

## Candidate Cases

- synthetic backend: `make_extern_global_array_load_module()`
- runtime: `tests/c/internal/backend_case/string_literal_char.c`
- runtime: `tests/c/internal/backend_case/global_char_pointer_diff.c`
- runtime: `tests/c/internal/backend_case/global_int_pointer_diff.c`
- runtime: `tests/c/internal/backend_case/global_int_pointer_roundtrip.c`

## Validation

- `tests/backend/backend_lir_adapter_tests.cpp` covers the exact global-addressing seam being implemented
- the promoted runtime case runs with `BACKEND_OUTPUT_KIND=asm`
- the backend-owned asm output assembles and preserves the expected runtime result for the bounded address-formation case
- the active AArch64 bring-up runbook remains narrow and does not silently absorb broader pointer/address lowering

## Good First Patch

Promote the smallest explicit global-addressing slice, likely `extern_global_array` or `string_literal_char`, onto the AArch64 asm path with a backend-owned base-address plus indexed-load seam that is validated in both synthetic backend tests and one runtime test.
