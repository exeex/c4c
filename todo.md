Status: Active
Source Idea Path: ideas/open/113_backend_struct_decl_layout_table_dual_path.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Convert Aggregate Size And Addressing Consumers

# Current Packet

## Just Finished

Step 3 - Convert Aggregate Size And Addressing Consumers converted dynamic
local aggregate array value load/store helper layout queries to prefer the
backend structured layout table when reached from the member dynamic local
aggregate load/store paths, with legacy compute-layout behavior preserved as
explicit no-table wrappers.

Changed files:

- `src/backend/bir/lir_to_bir/memory/local_slots.cpp`
- `src/backend/bir/lir_to_bir/lowering.hpp`
- `todo.md`
- `test_after.log`

Converted consumers:

- `load_dynamic_local_aggregate_array_value()` now has structured-table and
  null-table overloads and uses `lookup_scalar_byte_offset_layout()` for the
  dynamic element layout check.
- `append_dynamic_local_aggregate_store()` now has structured-table and
  null-table overloads and uses `lookup_scalar_byte_offset_layout()` for the
  dynamic element layout check.
- The member dynamic local aggregate load/store paths pass `structured_layouts_`
  through `try_lower_dynamic_local_aggregate_load()` and
  `try_lower_dynamic_local_aggregate_store()`.

Legacy behavior: the original no-structured-table helper overloads remain
available and forward through the same implementations with a null table,
preserving the existing `compute_aggregate_type_layout()` fallback path.

Remaining fallback-only Step 3 paths: no dynamic local aggregate array
load/store helper layout query remains direct-only fallback in this file set.
ABI, globals, initializers, unrelated MIR files, addressing edits, and test
expectations remain outside this packet.

## Suggested Next

Next coherent packet: send Step 3 to review, or delegate a separate bounded
packet for any remaining aggregate size/addressing consumer the supervisor
identifies outside the local slot dynamic aggregate array helpers.

## Watchouts

`ENABLE_C4C_BACKEND` remains off in the current `default` build tree, so the
first delegated build may report no work for backend objects. A pre-proof
backend-only compile rebuilt `memory/local_slots.cpp.o` successfully after the
signature threading.

Do not convert ABI, globals, initializers, or test expectations unless the
supervisor explicitly widens ownership.

## Proof

Delegated proof passed and wrote `test_after.log`:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|positive_split_llvm_|abi_)' && cmake --build build-backend --target c4c_backend -j) > test_after.log 2>&1`

Result: default build completed with no work, selected ctest subset passed 8/8,
and the backend `c4c_backend` target completed with no further work after the
pre-proof compile.
