# AArch64 Nested Member Pointer Array Plan

## Status

Completed on 2026-03-30.

## Relationship To Roadmap

Follows:

- `ideas/open/__backend_port_plan.md`
- `ideas/closed/15_backend_aarch64_param_member_array_plan.md`

## Goal

Promote the next bounded AArch64 backend-owned runtime seam after `param_member_array`: one pointer member load followed by one nested member-array indexed load through the asm path.

## Why This Is Separate

- `param_member_array` already proved the direct by-value member-array slice, but it intentionally excluded nested pointer-member chasing
- `nested_member_pointer_array` keeps the next slice narrow by adding exactly one extra pointer reload before the member-array access
- keeping this case separate avoids bundling nested by-value aggregate lowering or generic multi-level GEP support into the same runbook

## Candidate Case

- synthetic backend: `make_nested_member_pointer_array_gep_module()`
- runtime: `tests/c/internal/backend_case/nested_member_pointer_array.c`

## Validation Target

- tighten the synthetic AArch64 backend test around `make_nested_member_pointer_array_gep_module()` so the bounded lowered shape stays explicit
- promote `tests/c/internal/backend_case/nested_member_pointer_array.c` through `BACKEND_OUTPUT_KIND=asm` once the bounded backend-owned path is explicit and test-backed

## Non-Goals

- nested by-value aggregate access such as `nested_param_member_array`
- generic recursive GEP lowering beyond this exact outer-member-pointer to inner-array seam
- broader aggregate ABI changes
- built-in assembler or linker work

## Execution Notes

- prefer a matcher/emitter seam that recognizes one outer struct pointer parameter, one outer member-addressing GEP, one nested pointer load, one inner member-addressing GEP, one array-decay step, one indexed element GEP, one load, and one return
- compare the runtime-emitted LIR against `make_nested_member_pointer_array_gep_module()` before changing emission so the bounded contract is explicit
- stop and split again if implementation starts requiring generic nested aggregate lowering or support for the separate nested by-value seam

## Completion Notes

- tightened `make_nested_member_pointer_array_gep_module()` coverage to assert the bounded AArch64 asm helper shape instead of LLVM-text GEP preservation
- added a narrow AArch64 matcher/emitter for the helper-only synthetic slice and the exact two-function runtime module shape emitted for `nested_member_pointer_array`
- promoted `tests/c/internal/backend_case/nested_member_pointer_array.c` to `BACKEND_OUTPUT_KIND=asm`

## Validation

- targeted checks: `backend_lir_adapter_tests`, `backend_runtime_nested_member_pointer_array`, `backend_runtime_nested_param_member_array`, and `backend_runtime_param_member_array` all passed
- full-suite regression guard: `718 passed / 24 failed / 742 total` before and after, with zero newly failing tests

## Leftovers

- none; `nested_param_member_array` remains a separate non-goal and stays outside this closed slice
