# Backend Port Roadmap

## Status

Completed on 2026-03-30 and ready to archive as the umbrella roadmap for the native backend effort.

It is intentionally not the best direct activation target for `plan.md`.
Execution should usually activate one of the narrower backend ideas in `ideas/open/`.

Activation note on 2026-03-30:

- a temporary umbrella-backed activation was parked after confirming the previously targeted child `15_backend_aarch64_param_member_array_plan.md` had already been completed and moved to `ideas/closed/`
- do not recreate or reactivate child `15`; derive the next narrow child from the remaining backend roadmap gaps instead

## Goal

Port `ref/claudes-c-compiler/src/backend/` to `src/backend/` in C++ and make it consume the existing LIR in `src/codegen/lir/`.

Targets remain:

- AArch64
- x86-64
- rv64

Linux remains the execution target for produced binaries.

## Default Strategy

Default strategy: perform a mechanical C++ port of `ref/claudes-c-compiler/src/backend/` with minimal architectural deviation.

That means:

- prefer matching the ref backend's file structure, phase structure, and mechanism boundaries
- avoid redesigning shared abstractions up front
- avoid mixing cleanup with behavior changes
- treat the LIR attachment layer as the main intentional divergence from ref

## Why This Is Split

The original backend port plan mixed several distinct initiatives:

- LIR attachment and backend entry wiring
- AArch64 backend bring-up
- x86-64 backend bring-up
- rv64 backend bring-up
- built-in assembler work
- built-in linker work

That scope is too large for a single active plan under this repo's planning lifecycle.

## Current Open Child Ideas

The backend effort should keep execution focused on one narrow open child at a time.

Current open child:

- None. The final recorded child `19_backend_aarch64_return_zero_runtime_promotion_plan.md` was completed on 2026-03-30 and archived.

## Shared Guardrails

- Do not change existing LIR data structures just to make the first backend port easier.
- Prefer an adapter or shim over an early LIR redesign.
- Keep target-specific code isolated by architecture.
- Prefer external toolchain fallback over partial built-in encoding/linking.
- Keep patches mechanical where possible so ref/backend diffs stay reviewable.
- One mechanism family per patch.

## Priority Order

1. `01_backend_lir_adapter_plan.md`
2. `02_backend_aarch64_port_plan.md`
3. `03_backend_regalloc_peephole_port_plan.md`
4. `04_backend_binary_utils_contract_plan.md`
5. `05_backend_builtin_assembler_boundary_plan.md`
6. `06_backend_builtin_assembler_aarch64_plan.md`
7. `07_backend_linker_object_io_plan.md`
8. `08_backend_builtin_linker_aarch64_plan.md`

## Success Condition

This umbrella idea is complete when the child ideas are complete or superseded by narrower follow-up ideas.

## Current Follow-On Choice

The broad roadmap ordering above is mostly historical: `01` through `08` already completed as narrower child ideas, and the recent AArch64 global-addressing follow-ons `09`, `10`, `11`, `12`, and `13` are also closed.

The completed `13_backend_aarch64_global_int_pointer_diff_plan.md` child proved bounded scaled pointer subtraction over a mutable global `int` array.

The bounded AArch64 global int-pointer-roundtrip seam from `14_backend_aarch64_global_int_pointer_roundtrip_plan.md` is now closed after promoting that case through the backend-owned asm path.

The `15_backend_aarch64_param_member_array_plan.md` child was already completed and moved to `ideas/closed/` on 2026-03-30.

The `17_backend_aarch64_nested_member_pointer_array_plan.md` child was completed and moved to `ideas/closed/` on 2026-03-30 after promoting the bounded nested pointer-member chasing seam through the backend-owned asm path.

The `18_backend_aarch64_nested_param_member_array_plan.md` child was completed and moved to `ideas/closed/` on 2026-03-30 after promoting the bounded nested by-value aggregate member-array seam through the backend-owned asm path.

The final child `19_backend_aarch64_return_zero_runtime_promotion_plan.md` was completed on 2026-03-30 after confirming the bounded constant-return runtime seam already emitted through the backend-owned asm path and remained monotonic under full-suite regression validation.

No further backend child is recorded in this roadmap. The umbrella success condition is now satisfied and this file can move to `ideas/closed/`.

## Completion Notes

- all recorded backend child ideas referenced by this roadmap are now complete or archived
- the final runtime-harness promotion gap closed with child `19_backend_aarch64_return_zero_runtime_promotion_plan.md`
- no replacement backend child was identified during closure of `19`, so the umbrella roadmap no longer has an activatable follow-on slice
