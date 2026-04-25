Status: Active
Source Idea Path: ideas/open/113_backend_struct_decl_layout_table_dual_path.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Convert Aggregate Size And Addressing Consumers

# Current Packet

## Just Finished

Step 3 - Convert Aggregate Size And Addressing Consumers converted the memory
intrinsic aggregate repeated-extent paths by threading optional structured
layout access into the immediate local `memset`/`memcpy` helper surface.

Changed files:

- `src/backend/bir/lir_to_bir/lowering.hpp`
- `src/backend/bir/lir_to_bir/memory/intrinsics.cpp`
- `todo.md`
- `test_after.log`

Converted consumers: `try_lower_immediate_local_memset()` and
`try_lower_immediate_local_memcpy()` now have structured-aware overloads that
accept an optional `BackendStructuredLayoutTable*`, with legacy wrappers that
delegate through `nullptr`. Member callers in `lower_memory_memset_inst()`,
`lower_memory_memcpy_inst()`, and direct `memset`/`memcpy` runtime intrinsic
call lowering now pass `&structured_layouts_`.

The aggregate pointer-to-local-array recovery inside both helpers now calls the
structured `find_repeated_aggregate_extent_at_offset()` overload when a
structured table is available, and keeps the legacy extent lookup for wrapper
callers without structured layout access.

Remaining fallback-only paths: the intrinsic aggregate whole-object size/leaf
view helpers still use `lower_byval_aggregate_layout`; raw-byte slice scalar
leaf discovery still uses `resolve_scalar_layout_facts_at_byte_offset`;
`collect_local_scalar_array_slots` still uses the legacy local-slots helper;
static global-address helper wrappers, ABI and call lowering, globals,
initializers, and unrelated load/store-heavy lowering still use the legacy
layout path unless a caller explicitly opts into a structured overload.

## Suggested Next

Next coherent conversion packet: convert the remaining local scalar array
collection or intrinsic aggregate size/leaf-view fallback-only path by adding a
structured-aware overload while preserving legacy wrappers.

## Watchouts

`ENABLE_C4C_BACKEND` remains off in the current `default` build tree, so the
first delegated build may report no work for backend objects. The delegated
proof includes the backend-enabled `c4c_backend` target.

Do not convert ABI, globals, initializers, or load/store-heavy consumers unless
the supervisor explicitly widens ownership.

## Proof

Delegated proof passed and wrote `test_after.log`:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|positive_split_llvm_|abi_)' && cmake --build build-backend --target c4c_backend -j) > test_after.log 2>&1`

Result: default build completed with no work, selected ctest subset passed 8/8,
and `build-backend` rebuilt the backend objects needed by `c4c_backend`
successfully.
