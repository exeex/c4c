Status: Active
Source Idea Path: ideas/open/113_backend_struct_decl_layout_table_dual_path.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Convert Aggregate Size And Addressing Consumers

# Current Packet

## Just Finished

Step 3 - Convert Aggregate Size And Addressing Consumers converted the memory
addressing aggregate projection helper surface and selected `addressing.cpp`
consumers to prefer `lookup_backend_aggregate_type_layout` where structured
parity is available.

Changed files:

- `src/backend/bir/lir_to_bir/memory/addressing.cpp`
- `src/backend/bir/lir_to_bir/memory/memory_helpers.hpp`
- `todo.md`
- `test_after.log`

Converted consumers: `resolve_aggregate_byte_offset_projection()` and
`resolve_aggregate_child_index_projection()` now have structured-aware overloads
while preserving the legacy signatures for other memory files. In
`lower_memory_gep_inst()`, aggregate target sizing, array element sizing,
pointer-array length discovery, and repeated-aggregate extent checks that can
see `structured_layouts_` now use the structured-backed lookup path.

Remaining fallback-only paths: legacy projection signatures used by
`local_gep.cpp`, memory intrinsic consumers, static global-address helper
wrappers, `can_reinterpret_byte_storage_as_type`, ABI and call lowering,
globals, initializers, and unrelated load/store-heavy lowering still use the
legacy layout path unless a caller explicitly opts into the structured overload.

## Suggested Next

Next coherent conversion packet: convert a specifically named remaining
fallback-only memory consumer, most likely a local GEP or intrinsic path, by
threading the structured overload explicitly from the owning caller while
leaving unrelated legacy wrappers intact.

## Watchouts

The projection overloads intentionally return structured data only through
`lookup_backend_aggregate_type_layout`, so direct named type lookups with
checked matching parity can use structured layout and inline or unchecked cases
fall back through the existing legacy computation.

`ENABLE_C4C_BACKEND` remains off in the current `default` build tree, so the
first delegated build may report no work for backend objects. The delegated
proof includes the backend-enabled `c4c_backend` target.

Do not convert ABI, globals, initializers, or load/store-heavy consumers unless
the supervisor explicitly widens ownership.

## Proof

Delegated proof passed and wrote `test_after.log`:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|positive_split_llvm_|abi_)' && cmake --build build-backend --target c4c_backend -j) > test_after.log 2>&1`

Result: default build completed, selected ctest subset passed 8/8, and
`build-backend` completed the `c4c_backend` target.
