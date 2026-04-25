Status: Active
Source Idea Path: ideas/open/113_backend_struct_decl_layout_table_dual_path.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Convert Aggregate Size And Addressing Consumers

# Current Packet

## Just Finished

Step 3 - Convert Aggregate Size And Addressing Consumers completed the local
GEP aggregate layout/projection conversion by threading structured layout access
from `lower_memory_gep_inst()` into the local GEP helper surface.

Changed files:

- `src/backend/bir/lir_to_bir/lowering.hpp`
- `src/backend/bir/lir_to_bir/memory/addressing.cpp`
- `src/backend/bir/lir_to_bir/memory/local_gep.cpp`
- `src/backend/bir/lir_to_bir/memory/memory_helpers.hpp`
- `todo.md`
- `test_after.log`

Converted consumers: local GEP helper overloads now accept an optional
`BackendStructuredLayoutTable*`, and `addressing.cpp` passes
`&structured_layouts_` from local aggregate GEP lowering, dynamic local
aggregate projection, local slot pointer GEP, relative aggregate GEP target
resolution, and related local pointer-slot base paths. Legacy helper signatures
remain available as wrappers that pass `nullptr`.

Local GEP aggregate size/projection calls now route through
`lookup_backend_aggregate_type_layout`, structured
`resolve_aggregate_child_index_projection`, structured repeated-extent lookup,
and structured `can_reinterpret_byte_storage_as_type` when a structured table is
passed. Legacy-only callers continue to use `compute_aggregate_type_layout` and
the existing legacy projection signatures.

Remaining fallback-only paths: raw-byte slice scalar leaf discovery still uses
`resolve_scalar_layout_facts_at_byte_offset`, `collect_local_scalar_array_slots`
still uses the legacy local-slots helper, memory intrinsic consumers, static
global-address helper wrappers, ABI and call lowering, globals, initializers,
and unrelated load/store-heavy lowering still use the legacy layout path unless
a caller explicitly opts into a structured overload.

## Suggested Next

Next coherent conversion packet: convert the next specifically owned
fallback-only memory consumer, likely memory intrinsic aggregate extent paths or
the remaining local scalar array collection path, by adding a structured
overload and preserving legacy wrappers.

## Watchouts

`clang-format` is not installed in the container, so formatting was kept to the
existing local style by manual cleanup.

`ENABLE_C4C_BACKEND` remains off in the current `default` build tree, so the
first delegated build may report no work for backend objects. The delegated
proof includes the backend-enabled `c4c_backend` target.

Do not convert ABI, globals, initializers, or load/store-heavy consumers unless
the supervisor explicitly widens ownership.

## Proof

Delegated proof passed and wrote `test_after.log`:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|positive_split_llvm_|abi_)' && cmake --build build-backend --target c4c_backend -j) > test_after.log 2>&1`

Result: default build completed with no work, selected ctest subset passed 8/8,
and `build-backend` completed the `c4c_backend` target with no work after the
earlier compile check rebuilt the touched objects.
