Status: Active
Source Idea Path: ideas/open/113_backend_struct_decl_layout_table_dual_path.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Convert Aggregate Size And Addressing Consumers

# Current Packet

## Just Finished

Step 3 - Convert Aggregate Size And Addressing Consumers converted the memory
intrinsic aggregate whole-object size and leaf-view paths to prefer structured
layout when the optional backend structured layout table is available.

Changed files:

- `src/backend/bir/lir_to_bir/lowering.hpp`
- `src/backend/bir/lir_to_bir/memory/intrinsics.cpp`
- `todo.md`
- `test_after.log`

Converted consumers: `try_lower_immediate_local_memset()` and
`try_lower_immediate_local_memcpy()` now route aggregate whole-object layout
queries through `lower_intrinsic_aggregate_layout()`, which uses
`lookup_backend_aggregate_type_layout()` when callers provide a structured
layout table and keeps `lower_byval_aggregate_layout()` as the `nullptr`
wrapper fallback.

Converted call sites: `collect_sorted_leaf_slots_for_memops()`, whole-aggregate
`memset` size lookup, `build_memcpy_leaf_view_from_aggregate()`, and the
pointer-to-first-leaf aggregate best-match size comparison in `memcpy` leaf view
resolution.

Remaining fallback-only paths: legacy wrapper callers that pass `nullptr` to
the intrinsic helper surface intentionally keep `lower_byval_aggregate_layout`;
raw-byte slice scalar leaf discovery still uses
`resolve_scalar_layout_facts_at_byte_offset`; `collect_local_scalar_array_slots`
still uses the legacy local-slots helper; static global-address helper
wrappers, ABI and call lowering, globals, initializers, local slots, and
unrelated load/store-heavy lowering remain outside this packet.

## Suggested Next

Next coherent conversion packet: convert the remaining local scalar array
collection or raw-byte scalar leaf discovery path with the same
structured-when-available, legacy-wrapper-fallback shape.

## Watchouts

`ENABLE_C4C_BACKEND` remains off in the current `default` build tree, so the
first delegated build may report no work for backend objects. The delegated
proof includes the backend-enabled `c4c_backend` target.

Do not convert ABI, globals, initializers, local slots, or load/store-heavy
consumers unless the supervisor explicitly widens ownership.

## Proof

Delegated proof passed and wrote `test_after.log`:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|positive_split_llvm_|abi_)' && cmake --build build-backend --target c4c_backend -j) > test_after.log 2>&1`

Result: default build completed with no work, selected ctest subset passed 8/8,
and `build-backend` rebuilt the backend objects needed by `c4c_backend`
successfully.
