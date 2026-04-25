Status: Active
Source Idea Path: ideas/open/113_backend_struct_decl_layout_table_dual_path.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Convert Initializer And Aggregate Load/Store Consumers

# Current Packet

## Just Finished

Step 5 - Convert Initializer And Aggregate Load/Store Consumers converted the
global GEP address and dynamic global array addressing helpers to prefer the
backend structured layout table when invoked from member lowering paths, while
preserving the legacy no-table overloads.

Changed files:

- `src/backend/bir/lir_to_bir/lowering.hpp`
- `src/backend/bir/lir_to_bir/memory/addressing.cpp`
- `todo.md`
- `test_after.log`

Converted consumers:

- `resolve_global_gep_address()` now has a structured-aware overload and a
  nullable-table implementation that uses structured lookup for child
  projection and leaf layout.
- `resolve_relative_global_gep_address()` now has the same structured-aware
  path for global-pointer member GEPs and dynamic global aggregate projections.
- `resolve_global_dynamic_pointer_array_access()` now uses structured layout
  for aggregate traversal and pointer-array element layout when called from
  member lowering.
- `resolve_global_dynamic_aggregate_array_access()` now uses structured layout
  for base aggregate classification and repeated-aggregate extent lookup.
- `lower_memory_gep_inst()` now passes `structured_layouts_` at the global
  member-lowering call sites for direct globals, relative global pointers, and
  dynamic global aggregate leaf projection.

Remaining fallback-only paths: legacy public overloads for
`resolve_global_gep_address()`, `resolve_relative_global_gep_address()`,
`resolve_global_dynamic_pointer_array_access()`, and
`resolve_global_dynamic_aggregate_array_access()` still route through the
null-table layout path for callers without module structured layout state.

ABI classification files, global initializer files, globals/module files,
unrelated MIR files, test expectations, `plan.md`, and the source idea were not
changed.

## Suggested Next

Next coherent packet: send Step 5 to review, or delegate the next plan step the
supervisor selects after confirming this completes the structured-aware global
addressing helper conversion.

## Watchouts

The new overloads are intentionally plumbing-only: emitted behavior should stay
unchanged because structured lookup still falls back unless the backend table
has usable checked facts for the named aggregate type.

## Proof

Delegated proof passed and wrote `test_after.log`:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|positive_split_llvm_|abi_)' && cmake --build build-backend --target c4c_backend -j) > test_after.log 2>&1`

Result: default build completed, selected ctest subset passed 8/8, and the
backend `c4c_backend` target rebuilt successfully.
