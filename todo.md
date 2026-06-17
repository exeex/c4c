Status: Active
Source Idea Path: ideas/open/302_rv64_prepared_emitter_context_interface_cleanup.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Clean Frame And Local-Memory Support Boundaries

# Current Packet

## Just Finished

Step 5 cleaned the frame/local-memory support boundary without behavior
changes.

Changed files:
- `src/backend/mir/riscv/codegen/prepared_frame_emit.hpp`
- `src/backend/mir/riscv/codegen/prepared_frame_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_local_memory_emit.hpp`
- `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp`

Moved `emit_i32_load_from_stack_offset` and
`emit_i32_store_to_stack_offset` declarations/definitions from
`prepared_local_memory_emit` to `prepared_frame_emit`. The helper bodies were
moved unchanged, including unaligned bytewise load/store emission. Local-memory
continues to call the helpers through frame/support ownership, and scalar no
longer includes local-memory solely for stack helper access.

## Suggested Next

Review whether Step 5 has any remaining support-boundary cleanup or move to
the next plan step selected by the supervisor.

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
- `simple_frame_slot_sp_offset_for` remains in frame support; this packet only
  moved the shared stack-offset load/store helpers to the same ownership area.

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
