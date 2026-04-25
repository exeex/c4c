Status: Active
Source Idea Path: ideas/open/113_backend_struct_decl_layout_table_dual_path.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Convert Aggregate Size And Addressing Consumers

# Current Packet

## Just Finished

Step 3 - Convert Aggregate Size And Addressing Consumers converted the local
aggregate slot declaration/scalar-slot layout path in `aggregate.cpp` to prefer
`lookup_backend_aggregate_type_layout` where structured parity is available.

Changed files:

- `src/backend/bir/lir_to_bir/aggregate.cpp`
- `todo.md`
- `test_after.log`

Converted consumers: `declare_local_aggregate_slots()` now uses the structured
lookup for the aggregate declaration layout, and
`append_local_aggregate_scalar_slots()` now uses the structured lookup for each
recursive scalar-slot layout lookup and array element stride lookup.

Remaining fallback-only paths: `lower_byval_aggregate_layout` and its
aggregate-copy size checks, ABI and call lowering, globals, memory addressing,
initializers, local/global load/store-heavy lowering, and memory intrinsic
consumers still use the legacy layout path.

## Suggested Next

Next coherent conversion packet: decide whether the byval aggregate layout path
in `aggregate.cpp` should also receive the structured lookup, or leave
aggregate.cpp and route the next non-ABI/non-global consumer named by the
supervisor through the helper.

## Watchouts

The helper intentionally returns structured data only for direct named type
lookups whose existing table entry has matching checked parity. Inline structs,
array wrapper layouts, missing structured names, unchecked entries, and
mismatches remain legacy-computed to preserve current output; named array
elements can use structured data only after the array layer has resolved to the
element type text.

`ENABLE_C4C_BACKEND` remains off in the current `default` build tree, so the
first delegated build may report no work for backend objects. The delegated
proof includes the backend-enabled `c4c_backend` target and rebuilt
`aggregate.cpp.o` successfully.

Do not convert ABI, globals, memory addressing, initializers, or load/store
heavy consumers unless the supervisor explicitly widens ownership.

## Proof

Delegated proof passed and wrote `test_after.log`:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|positive_split_llvm_|abi_)' && cmake --build build-backend --target c4c_backend -j) > test_after.log 2>&1`

Result: default build completed, selected ctest subset passed 8/8, and
`build-backend` rebuilt `aggregate.cpp.o` and linked `src/backend/c4c_backend.a`.
