# AArch64 Nested Param Member Array Plan

## Status

Completed on 2026-03-30 and ready to archive.

## Relationship To Roadmap

Follows:

- `ideas/open/__backend_port_plan.md`
- `ideas/closed/17_backend_aarch64_nested_member_pointer_array_plan.md`

## Goal

Promote the next bounded AArch64 backend-owned runtime seam after `nested_member_pointer_array`: one by-value nested aggregate parameter whose inner array member is indexed and loaded through the asm path.

## Why This Is Separate

- `nested_member_pointer_array` already proved the adjacent pointer-member chasing seam, but it intentionally left nested by-value aggregate access out of scope
- `nested_param_member_array` keeps the next slice narrow by reusing the same member-array access pattern without the extra pointer reload, while still requiring the backend-owned asm path to handle the nested by-value aggregate shape
- keeping this case separate avoids bundling generic aggregate lowering, recursive GEP support, or broader ABI redesign into the same runbook

## Candidate Case

- synthetic backend: `make_nested_param_member_array_gep_module()`
- runtime: `tests/c/internal/backend_case/nested_param_member_array.c`

## Validation Target

- tighten the synthetic AArch64 backend test around `make_nested_param_member_array_gep_module()` so the bounded lowered shape stays explicit
- promote `tests/c/internal/backend_case/nested_param_member_array.c` through `BACKEND_OUTPUT_KIND=asm` once the bounded backend-owned path is explicit and test-backed

## Non-Goals

- generic recursive GEP lowering beyond this exact nested by-value member-array seam
- broader aggregate ABI changes
- built-in assembler or linker work
- unrelated x86-64 or rv64 backend work

## Execution Notes

- prefer a matcher/emitter seam that recognizes one spilled by-value outer struct parameter, one outer member-addressing GEP, one nested inner member-addressing GEP, one array-decay step, one indexed element GEP, one load, and one return
- compare the runtime-emitted LIR against `make_nested_param_member_array_gep_module()` before changing emission so the bounded contract is explicit
- stop and split again if implementation starts requiring generic aggregate flattening, recursive lowering outside this exact seam, or unrelated ABI work

## Success Condition

This idea is complete when the synthetic AArch64 backend test and the runtime `nested_param_member_array` case both pass through the bounded backend-owned asm path with no newly failing regressions.

## Completion Notes

- updated `make_nested_param_member_array_gep_module()` so the synthetic helper matches the real runtime LIR seam, including the nested `[2 x i32]` decay step before the indexed element GEP
- added the bounded AArch64 matcher/emitter support for the nested by-value aggregate helper and runtime module shapes in `src/backend/aarch64/codegen/emit.cpp`
- promoted `tests/c/internal/backend_case/nested_param_member_array.c` to `BACKEND_OUTPUT_KIND=asm`
- validated the slice with `backend_lir_adapter_tests`, `backend_runtime_nested_param_member_array`, nearby backend runtime cases, and a full-suite before/after comparison

## Leftover Issues

- no slice-specific leftovers were discovered; the pre-existing full-suite failures remained unchanged at 24 failing tests before and after the patch
