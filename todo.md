Status: Active
Source Idea Path: ideas/open/185_lir_to_bir_global_typedecl_compatibility_fence.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory global/type/layout lookup surfaces

# Current Packet

## Just Finished

Completed Step 1 source audit for `GlobalTypes`, `TypeDeclMap`, and
`BackendStructuredLayoutTable` lookup authority.

Inventory:
- `TypeDeclMap` is built by `build_type_decl_map` in
  `src/backend/bir/lir_to_bir/types.cpp` from `LirModule::type_decls`; it is
  keyed by final type spelling and feeds legacy layout parsing through
  `resolve_type_decl_body`, `compute_aggregate_type_layout`, and callers of
  `lookup_backend_aggregate_type_layout_result`.
- `BackendStructuredLayoutTable` is built by
  `build_backend_structured_layout_table` in
  `src/backend/bir/lir_to_bir/types.cpp` from `LirModule::struct_decls`,
  `StructNameTable`, and the legacy `TypeDeclMap`; entries carry
  `StructNameId`, structured layout, optional legacy layout, and parity status.
- `GlobalTypes` is constructed in `lower_module` in
  `src/backend/bir/lir_to_bir/module.cpp`; ordinary globals are keyed by
  `global_name_for_identity` (`LinkNameId` spelling when present, raw name
  otherwise), string-pool constants remain raw-name only, and each `GlobalInfo`
  retains `link_name_id`, storage/layout facts, compatibility type text, and
  pointer-initializer facts.
- Existing structured fact sources are `LirGlobal::link_name_id`,
  `LirGlobal::llvm_type_ref`, `LirGlobal::initializer_function_link_name_ids`,
  `LirStructDecl::name_id`/fields, `LirTypeRef::struct_name_id`, the module
  `LinkNameTable`, and the module `StructNameTable`.
- Generated metadata-rich routes already present: global/function identity
  naming via `LinkNameId`; aggregate global layout via
  `LirGlobal::llvm_type_ref` to
  `lookup_backend_aggregate_type_ref_layout`; aggregate initializer lowering
  via `lower_aggregate_initializer_for_type_ref`; direct-call/signature
  aggregate lookup via `LirTypeRef`; pointer initializer function targets via
  `initializer_function_link_name_ids`.
- Raw/no-id compatibility routes still present: `TypeDeclMap` textual layout
  fallback; `lookup_backend_aggregate_type_layout_result(text, ...)` fallback
  from final spelling to legacy type decls; `parse_global_address_initializer`
  parsing raw `@symbol`/GEP text; `FunctionSymbolSet::find_raw_symbol_link_name_id`;
  string-pool globals; `GlobalTypes::find(raw_name)` lookups used by memory,
  provenance, pointer initializer, and direct global-address lowering.
- Fallback/consumer hotspots to classify in Step 2: `lower_minimal_global_impl`
  chooses `llvm_type_ref` when present but otherwise falls back through
  `global.llvm_type`; `lower_aggregate_initializer_recursive` and
  `parse_global_address_initializer_impl` parse raw symbol/GEP text; module
  initializer resolution falls back from absent function id to `GlobalTypes` and
  raw function-symbol lookup; memory consumers in
  `memory/addressing.cpp`, `memory/local_slots.cpp`, `memory/provenance.cpp`,
  and `memory/value_materialization.cpp` use raw global names as storage handles
  while preserving `LinkNameId` on resolved BIR values.

## Suggested Next

Execute Step 2 of `plan.md`: select one generated metadata-rich
global/type/layout route, require the structured identity facts already present
there, and keep the raw/no-id routes explicitly fenced as compatibility.

## Watchouts

- Idea 185 is a boundary audit plus selected generated-path repair; do not
  rewrite all legacy type declaration parsing in one pass.
- Preserve printer/final spelling and raw imported LIR compatibility unless a
  selected generated metadata-rich path proves it must fail closed.
- The most direct Step 2 candidates are aggregate globals with
  `LirGlobal::llvm_type_ref` and pointer initializers carrying
  `initializer_function_link_name_ids`; both already have structured metadata
  and nearby raw fallback paths to fence.
- `BackendStructuredLayoutTable` is still keyed by final type spelling for the
  structured-to-legacy bridge, but `lookup_backend_aggregate_type_ref_layout_result`
  selects entries by `StructNameId`; prefer extending that route over adding new
  string-keyed generated-path authority.
- Backend freeze closure remains owned by idea 188.

## Proof

Source-level audit only. Validation command:
`git diff --check -- todo.md`

Result: passed.
