Status: Active
Source Idea Path: ideas/open/113_backend_struct_decl_layout_table_dual_path.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce Structured Backend Layout Table

# Current Packet

## Just Finished

Step 2 - Introduce Structured Backend Layout Table completed the BIR
lookup-authority slice and tightened it with observable parity mismatch
reporting.

Changed files:

- `src/backend/bir/lir_to_bir/lowering.hpp`
- `src/backend/bir/lir_to_bir/types.cpp`
- `src/backend/bir/lir_to_bir/module.cpp`
- `todo.md`

The new `BackendStructuredLayoutTable` is built from
`context.lir_module.struct_decls` and `context.lir_module.struct_names` in
`lower_module`, immediately next to the existing
`build_type_decl_map(context.lir_module.type_decls)` flow. Entries carry the
structured `AggregateTypeLayout`, the legacy `compute_aggregate_type_layout`
result when a legacy declaration exists, and parity status.

Checked structured-vs-legacy parity mismatches now report through the existing
BIR lowering note mechanism with `context.note("module", ...)`, including the
type name, legacy size/align/fields, and structured size/align/fields.
Matching entries remain silent.

Fallback/output behavior is unchanged: all ABI, globals, memory, load/store,
and initializer consumers still receive the legacy `TypeDeclMap`, and no
consumer has been converted to read the structured table yet.

## Suggested Next

Next coherent conversion packet: add a lookup helper that accepts a type name
and the `BackendStructuredLayoutTable`, returns legacy-equivalent
`AggregateTypeLayout` facts through the legacy fallback, and routes mismatch
notes through the shared reporting path without changing consumer output. Start
with a narrow lookup-only caller before touching ABI or memory lowering.

## Watchouts

Do not remove `module.type_decls`; structured layout must keep legacy fallback
and parity observation.

`ENABLE_C4C_BACKEND` is off in the current `default` build tree, so the
delegated proof build did not recompile backend objects. `build-backend/`
exists and `cmake --build build-backend --target c4c_backend -j` rebuilt the
touched backend objects successfully.

Do not start with ABI or memory lowering. Keep the next packet at the shared
lookup-helper boundary so later consumers share one fallback/parity surface.

## Proof

Delegated proof passed and wrote `test_after.log`:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|positive_split_llvm_|abi_)') > test_after.log 2>&1`

Result: build command completed, selected ctest subset passed 8/8.

Backend-enabled proof also passed:

`cmake --build build-backend --target c4c_backend -j`

Result: rebuilt `c4c_backend`, including `types.cpp.o` and `module.cpp.o`,
and linked `src/backend/c4c_backend.a`.
