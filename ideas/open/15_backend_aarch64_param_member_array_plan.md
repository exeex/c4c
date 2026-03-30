# AArch64 Param Member Array Plan

## Status

Open.

## Relationship To Roadmap

Follows:

- `ideas/open/__backend_port_plan.md`
- `ideas/closed/14_backend_aarch64_global_int_pointer_roundtrip_plan.md`

## Goal

Promote the next bounded AArch64 backend-owned runtime seam after the global-addressing follow-ons: one by-value struct parameter whose array member is indexed and loaded through the asm path.

## Why This Is Separate

- the completed `global_int_pointer_roundtrip` slice closed the recent bounded global-addressing series
- `param_member_array` is adjacent backend work, but it exercises a different seam: by-value aggregate parameter spilling plus member-array decay and indexed element load
- keeping this slice separate preserves the narrow-slice rule and avoids bundling nested pointer chasing or broader aggregate lowering into one runbook

## Candidate Case

- synthetic backend: `make_param_member_array_gep_module()`
- runtime: `tests/c/internal/backend_case/param_member_array.c`

## Validation Target

- tighten the existing synthetic AArch64 backend test around `make_param_member_array_gep_module()` so the expected lowered shape stays explicit
- promote `tests/c/internal/backend_case/param_member_array.c` through `BACKEND_OUTPUT_KIND=asm` once the bounded backend-owned path is explicit and test-backed

## Non-Goals

- nested aggregate member traversal
- pointer-member chasing such as `nested_member_pointer_array`
- nested by-value aggregate access such as `nested_param_member_array`
- generic aggregate copies or ABI expansion beyond this exact slice
- built-in assembler or linker work

## Execution Notes

- prefer a matcher/emitter seam that recognizes one spilled by-value `%struct.Pair` parameter, one member-addressing GEP, one array-decay GEP, one indexed element GEP, one load, and one return
- compare the runtime-emitted LIR against `make_param_member_array_gep_module()` before changing emission so the bounded contract is explicit
- stop and split again if implementation starts requiring nested aggregate flattening, generic GEP lowering, or unrelated ABI work

## Progress

- Synthetic AArch64 coverage and bounded emission support for the exact `param_member_array` runtime-shaped slice are implemented.
- `tests/c/internal/backend_case/param_member_array.c` is promoted to `BACKEND_OUTPUT_KIND=asm` and targeted backend/runtime checks passed during execution.

## Current Closure Blocker

- Resolved on 2026-03-30 via `ideas/closed/16_cpp_eastl_initializer_list_runtime_validation_blocker.md`.
- Clean-build regression validation now passes again with no newly failing tests:
  - before: passed=715 failed=25 total=740
  - after: passed=718 failed=24 total=742
