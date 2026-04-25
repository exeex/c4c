Status: Active
Source Idea Path: ideas/open/113_backend_struct_decl_layout_table_dual_path.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Convert Aggregate Size And Addressing Consumers

# Current Packet

## Just Finished

Step 3 - Convert Aggregate Size And Addressing Consumers converted loaded local
pointer-slot state recording to prefer the backend structured layout table when
the caller has one, with the legacy compute-layout path preserved as an
explicit wrapper.

Changed files:

- `src/backend/bir/lir_to_bir/memory/local_slots.cpp`
- `src/backend/bir/lir_to_bir/lowering.hpp`
- `todo.md`
- `test_after.log`

Converted consumer: `record_loaded_local_pointer_slot_state()` now has a
structured-aware overload that receives `structured_layouts_` from
`try_lower_tracked_local_pointer_slot_load()` and uses the shared
`lookup_scalar_byte_offset_layout()` path for both the loaded aggregate layout
and nested element layout queries.

Legacy behavior: the original no-structured-table overload remains available
and forwards through the same implementation with a null table, preserving the
existing `compute_aggregate_type_layout()` fallback path.

Remaining fallback-only paths: dynamic local aggregate array value load/store
helpers still call `compute_aggregate_type_layout()` directly because they were
explicitly outside this packet unless required for compilation. ABI, globals,
initializers, unrelated MIR files, addressing edits, and test expectations
remain outside this packet.

## Suggested Next

Next coherent packet: send Step 3 to review, or delegate a separate bounded
packet for the dynamic local aggregate array value load/store helpers if the
supervisor wants those fallback-only local slot paths converted.

## Watchouts

`ENABLE_C4C_BACKEND` remains off in the current `default` build tree, so the
first delegated build may report no work for backend objects. The delegated
proof includes the backend-enabled `c4c_backend` target and rebuilt
`memory/local_slots.cpp.o`.

Do not convert ABI, globals, initializers, or test expectations unless the
supervisor explicitly widens ownership.

## Proof

Delegated proof passed and wrote `test_after.log`:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|positive_split_llvm_|abi_)' && cmake --build build-backend --target c4c_backend -j) > test_after.log 2>&1`

Result: default build completed with no work, selected ctest subset passed 8/8,
and `build-backend` rebuilt the backend objects needed by `c4c_backend`,
including `memory/local_slots.cpp.o`, successfully.
