Status: Active
Source Idea Path: ideas/open/302_rv64_prepared_emitter_context_interface_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce A Narrow Prepared Emit Context

# Current Packet

## Just Finished

Step 2 introduced the first narrow prepared lookup support surface without
behavior changes.

Changed files:
- `src/backend/mir/riscv/codegen/prepared_emit_context.hpp`
- `src/backend/mir/riscv/codegen/prepared_emit_context.cpp`
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.hpp`
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_function_emit.cpp`
- `src/backend/CMakeLists.txt`

Moved lookup/support declarations and definitions out of scalar's public helper
surface into `prepared_emit_context.{hpp,cpp}`:
`prepared_value_home_for`, `prepared_register_for_value`,
`prepared_pointer_register_for_value`,
`prepared_register_for_value_name_id`, and
`has_frame_slot_address_materialization_at`.

`prepared_scalar_emit.hpp` now keeps scalar owner/emission declarations and the
still-scalar `emit_move_to_register`; local-memory and function owners include
the new context header for direct lookup helper use. The scalar private
rematerializable-immediate query now reuses `prepared_value_home_for` instead of
owning a duplicate prepared value-id lookup.

## Suggested Next

Delegate the next context cleanup packet only if the supervisor wants to
continue reducing Step 4 raw current-location parameter bundles: introduce an
explicit owner-constructed lookup/context handle around
`PreparedNameTables`, `PreparedFunctionLookups*`, `BlockLabelId`, and
instruction index at the scalar/local-memory/function call sites, keeping block
and instruction ownership visible.

## Watchouts

- Keep this idea behavior-preserving; do not add RV64 runtime cases or broaden
  backend capability.
- Do not weaken tests, expectations, unsupported handling, or qemu runtime
  coverage.
- Avoid creating a central support file that becomes a miscellaneous helper
  pile.
- `emit_move_to_register` is shared by scalar, call, and local-memory, but it
  emits assembly and depends on stack-load helpers; leave it behind until the
  lookup-only support surface exists or move it only if the supervisor explicitly
  accepts that broader coupling.
- `emit_i32_load_from_stack_offset`, `emit_i32_store_to_stack_offset`, and
  `simple_frame_slot_sp_offset_for` are better handled by the later
  frame/local-memory boundary step, not the first lookup extraction.

## Proof

Ran exactly:
`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -R '\''^backend_rv64_runtime'\'' --output-on-failure; } > test_after.log 2>&1'`

Result: passed. `test_after.log` contains a successful build and 23/23 passing
`backend_rv64_runtime` tests.
