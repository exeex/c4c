Status: Active
Source Idea Path: ideas/open/113_backend_struct_decl_layout_table_dual_path.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Convert Aggregate Size And Addressing Consumers

# Current Packet

## Just Finished

Step 3 - Convert Aggregate Size And Addressing Consumers started the
structured-layout lookup conversion by adding the shared
`lookup_backend_aggregate_type_layout` helper and routing the first
aggregate.cpp consumer through it.

Changed files:

- `src/backend/bir/lir_to_bir/lowering.hpp`
- `src/backend/bir/lir_to_bir/types.cpp`
- `src/backend/bir/lir_to_bir/module.cpp`
- `src/backend/bir/lir_to_bir/aggregate.cpp`
- `todo.md`

The helper accepts a type name/text plus the legacy `TypeDeclMap` and
`BackendStructuredLayoutTable`. It returns the structured layout only when a
named structured entry has checked parity and matches the legacy layout; all
other inputs, missing entries, unchecked entries, and mismatches fall back to
`compute_aggregate_type_layout`.

Converted consumer: `BirFunctionLowerer::collect_sorted_leaf_slots()` now uses
`lookup_backend_aggregate_type_layout` for the aggregate slot extent it needs
when sorting leaves. `BirFunctionLowerer` now carries the structured layout
table from `lower_module` to support this member-only lookup.

Remaining fallback-only paths: `lower_byval_aggregate_layout`,
`append_local_aggregate_scalar_slots`, `declare_local_aggregate_slots`, ABI and
call lowering, globals, memory addressing, initializers, local/global
load/store-heavy lowering, and memory intrinsic consumers still use the legacy
layout path.

## Suggested Next

Next coherent conversion packet: convert one more aggregate.cpp-only consumer
that has narrow behavior and no ABI/global/memory-addressing ownership, such
as another leaf-slot layout extent check, through
`lookup_backend_aggregate_type_layout`.

## Watchouts

The helper intentionally returns structured data only for direct named type
lookups whose existing table entry has matching checked parity. Inline structs,
arrays, missing structured names, unchecked entries, and mismatches remain
legacy-computed to preserve current output.

`ENABLE_C4C_BACKEND` remains off in the current `default` build tree, so the
delegated proof build does not recompile backend objects. `build-backend/`
exists and `cmake --build build-backend --target c4c_backend -j` rebuilt the
touched backend objects successfully.

Do not convert ABI, globals, memory addressing, initializers, or load/store
heavy consumers in the next narrow aggregate.cpp packet unless the supervisor
explicitly widens ownership.

## Proof

Delegated proof passed and wrote `test_after.log`:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|positive_split_llvm_|abi_)') > test_after.log 2>&1`

Result: build command completed, selected ctest subset passed 8/8.

Backend-enabled proof also passed:

`cmake --build build-backend --target c4c_backend -j`

Result: rebuilt `c4c_backend`, including `aggregate.cpp.o`, `types.cpp.o`, and
`module.cpp.o`, and linked `src/backend/c4c_backend.a`.
