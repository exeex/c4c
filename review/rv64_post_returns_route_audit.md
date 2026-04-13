# RV64 Post-Returns Route Audit

Date: 2026-04-12
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- commit `63245b3a` (`rv64: land returns entry body`)

## Validation

- `cmake --build build --target backend_shared_util_tests c4cll`
  - log: `rv64_returns_entry_supervisor_build.log`
- `./build/backend_shared_util_tests riscv_translated_returns`
  - log: `rv64_returns_entry_supervisor_shared_util.log`
- `./build/backend_shared_util_tests riscv`
  - log: `rv64_returns_entry_supervisor_riscv_suite.log`

All three commands exited successfully.

## Findings

- The bounded `returns.cpp::emit_return_impl` entry body is now real in-tree and
  covered by focused shared-util tests.
- `src/backend/riscv/codegen/mod.cpp` is still only a structural mirror, but
  making it the next packet would not move a new translated owner body behind
  the shared RV64 seam.
- `src/backend/riscv/codegen/calls.cpp` currently carries duplicated placeholder
  ABI and operand/type surfaces; routing there next would widen immediately into
  broader shared backend state.
- `src/backend/riscv/codegen/memory.cpp` contains the next smallest translated
  helper set that can advance behind the current RV64 seam without claiming the
  full memory owner is ready.

## Route Decision

Select the first bounded `memory.cpp` helper seam as the next packet, limited
to helpers that only depend on the already-landed shared RV64 surface:

- `emit_typed_store_to_slot_impl(...)`
- `emit_typed_load_from_slot_impl(...)`
- `emit_add_offset_to_addr_reg_impl(...)`
- `emit_add_imm_to_acc_impl(...)`

Keep the rest of `memory.cpp` parked for now:

- default load/store delegation
- pointer-resolution helpers that need broader state
- `memcpy` loops
- aligned-allocation address helpers
- full GEP owner bodies

## Rationale

This route preserves the integration-first shape of the lane. It advances a
real translated owner file, reuses the existing `StackSlot`, `IrType`,
`RiscvCodegenState`, and s0-relative helper surface, and avoids widening into
the broader `RiscvCodegen` / shared-state work that the call and full memory
owners still require.
