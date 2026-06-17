Status: Active
Source Idea Path: ideas/open/302_rv64_prepared_emitter_context_interface_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Reduce Current-Location Parameter Bundles

# Current Packet

## Just Finished

Step 4 reduced repeated current-location prepared lookup bundles without
behavior changes.

Changed files:
- `src/backend/mir/riscv/codegen/prepared_emit_context.hpp`
- `src/backend/mir/riscv/codegen/prepared_emit_context.cpp`
- `src/backend/mir/riscv/codegen/prepared_call_emit.hpp`
- `src/backend/mir/riscv/codegen/prepared_call_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.hpp`
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_local_memory_emit.hpp`
- `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_function_emit.cpp`

Added `PreparedCurrentInstructionContext`, an explicit handle carrying
`PreparedNameTables`, `PreparedFunctionLookups*`, `BlockLabelId`, and the
current instruction index. The function emitter constructs it at the instruction
ownership point, including an explicit lookahead context for the adjacent
instruction skip check.

Updated scalar pointer-add/select helpers, local-memory load/store helpers, and
the simple call helper to consume the context where it replaces repeated
current-location parameter bundles. Block index ownership remains explicit for
prepared call-plan lookup, and frame/local-memory stack helper ownership was
left unchanged.

## Suggested Next

Delegate the next packet to continue the active cleanup at the
frame/local-memory boundary only if the supervisor wants to address the stack
helper ownership called out by the plan; otherwise review whether the current
context surface is now narrow enough before moving more emission helpers.

## Watchouts

- Keep this idea behavior-preserving; do not add RV64 runtime cases or broaden
  backend capability.
- Do not weaken tests, expectations, unsupported handling, or qemu runtime
  coverage.
- The new context is intentionally current-instruction scoped; do not turn it
  into a catch-all function or module emission context without plan review.
- `emit_move_to_register` still takes `names` and `lookups` because it is also
  used by non-current terminator/cast paths. Moving it would require a separate
  ownership decision.
- `emit_i32_load_from_stack_offset`, `emit_i32_store_to_stack_offset`, and
  `simple_frame_slot_sp_offset_for` remain untouched beyond call-site
  adaptation.

## Proof

Ran exactly:
`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -R '\''^backend_rv64_runtime'\'' --output-on-failure; } > test_after.log 2>&1'`

Result: passed. `test_after.log` contains a successful build and 23/23 passing
`backend_rv64_runtime` tests.

Supervisor validation:
`ctest --test-dir build -R 'backend_riscv|backend_codegen_route_riscv64' --output-on-failure`

Result: preserved the documented baseline failure
`backend_riscv_prepared_edge_publication`; both
`backend_codegen_route_riscv64` tests passed.
