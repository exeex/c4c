# RV64 Post-Dynamic-Alloca Route Audit

Date: 2026-04-12
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- commit `0b497a32` (`rv64: land dynamic alloca helper seam`)

## Validation

- `cmake --build build --target backend_shared_util_tests c4cll`
  - log: `rv64_dynamic_alloca_build.log`
- `./build/backend_shared_util_tests riscv_translated_memory_helper`
  - log: `rv64_dynamic_alloca_shared_util.log`
- `./build/backend_shared_util_tests riscv_codegen_header_exports_translated_memory_helper`
  - log: `rv64_dynamic_alloca_header_shared_util.log`

All three commands exited successfully.

## Findings

- The bounded translated RV64 dynamic-alloca helper seam is now real in-tree
  and covered by focused shared-util checks.
- The remaining pointer-resolution, GEP, and `memcpy` helper families in
  `src/backend/riscv/codegen/memory.cpp` still depend on reg-assignment or
  broader shared RV64 state that is intentionally parked by the active plan.
- The remaining aligned-address helpers in `memory.cpp` are the next smallest
  translated seam. They still need a narrow shared-state follow-through for
  over-aligned alloca metadata, but they avoid the wider pointer-resolution
  and call-ABI surface that the other parked families would pull in.

## Route Decision

Select the bounded translated aligned-address helper seam in
`src/backend/riscv/codegen/memory.cpp` as the next packet, limited to:

- `emit_alloca_aligned_addr_impl(...)`
- `emit_alloca_aligned_addr_to_acc_impl(...)`

Allow only the minimum paired shared-surface touches those two helpers
strictly require for over-aligned alloca metadata and bounded s0-relative
address formation.

Keep the rest of `memory.cpp` parked for now:

- pointer-resolution helpers
- GEP owner bodies
- `memcpy` helpers
- broader shared RV64 state expansion beyond the aligned-address helper need
- call ABI work

## Rationale

This route keeps the lane integration-first and packet-bounded. It continues
inside the active translated RV64 `memory.cpp` owner file, but it does so with
the smallest remaining helper seam that can advance without turning the plan
into broad pointer-resolution or call-surface bring-up.
