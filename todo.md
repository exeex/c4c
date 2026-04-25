Status: Active
Source Idea Path: ideas/open/113_backend_struct_decl_layout_table_dual_path.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Convert Aggregate Size And Addressing Consumers

# Current Packet

## Just Finished

Step 3 - Convert Aggregate Size And Addressing Consumers converted local scalar
array slot collection to prefer the backend structured layout table when the
caller has one, with the legacy compute-layout path preserved as an explicit
wrapper.

Changed files:

- `src/backend/bir/lir_to_bir/memory/local_slots.cpp`
- `src/backend/bir/lir_to_bir/memory/addressing.cpp`
- `src/backend/bir/lir_to_bir/lowering.hpp`
- `todo.md`
- `test_after.log`

Converted consumers: `collect_local_scalar_array_slots()` now has a
structured-aware overload that uses `lookup_backend_aggregate_type_layout()`
and the structured `find_nested_repeated_aggregate_extent_at_offset()` path.
The original three-argument helper remains as the legacy fallback wrapper.

Converted call sites: local aggregate GEP scalar-array publication in
`addressing.cpp` and local pointer-slot address propagation in
`local_slots.cpp` now pass `structured_layouts_` into scalar array slot
collection. The nearby local aggregate target layout query in pointer-slot
store propagation also uses `lookup_backend_aggregate_type_layout()`.

Remaining fallback-only paths: legacy callers of the three-argument scalar
array slot helper still use `compute_aggregate_type_layout()` through the
wrapper; `record_loaded_local_pointer_slot_state()` and dynamic local aggregate
array value load/store helpers remain fallback-only because they do not receive
`structured_layouts_`; ABI, globals, initializers, and test expectations remain
outside this packet.

## Suggested Next

Next coherent packet: convert the remaining fallback-only local slot helpers
that currently lack a structured-layout parameter, or send Step 3 to review if
the supervisor considers local aggregate size/addressing consumers sufficiently
covered.

## Watchouts

`ENABLE_C4C_BACKEND` remains off in the current `default` build tree, so the
first delegated build may report no work for backend objects. The delegated
proof includes the backend-enabled `c4c_backend` target.

Do not convert ABI, globals, initializers, or test expectations unless the
supervisor explicitly widens ownership.

## Proof

Delegated proof passed and wrote `test_after.log`:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|positive_split_llvm_|abi_)' && cmake --build build-backend --target c4c_backend -j) > test_after.log 2>&1`

Result: default build completed with no work, selected ctest subset passed 8/8,
and `build-backend` rebuilt the backend objects needed by `c4c_backend`
successfully.
