# Current Packet

Status: Active
Source Idea Path: ideas/open/138_lir_bir_backend_aggregate_layout_type_decl_text_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Trace Current Aggregate Layout Authority

## Just Finished

Step 1 trace completed for idea 138. Current aggregate layout authority routes:

- Structured-primary: HIR-to-LIR populates `LirModule::struct_decls` with
  `LirStructDecl` entries keyed by `StructNameId` while still emitting
  `LirModule::type_decls` as legacy shadow text. BIR import builds
  `module.structured_types` through `build_bir_structured_type_spelling_context()`
  for final spelling/display, and builds `BackendStructuredLayoutTable` through
  `build_backend_structured_layout_table()` for semantic layout. Most BIR
  lowering consumers that receive `structured_layouts` call
  `lookup_backend_aggregate_type_layout()` before falling back to
  `compute_aggregate_type_layout()`, including byval aggregate ABI lowering,
  local aggregate slots, aggregate leaf-slot expansion, global aggregate
  storage sizing, aggregate initializers, and global-address initializer GEP
  parsing.
- Fallback-only: raw or hand-built LIR without `struct_decls` still flows
  through `TypeDeclMap build_type_decl_map(context.lir_module.type_decls)` and
  `compute_aggregate_type_layout()`, which resolves `%type` names through the
  textual type-declaration body and recursively parses arrays/struct bodies.
  This fallback is still required for legacy tests and compatibility fixtures.
- Suspicious text-authority routes: `lookup_backend_aggregate_type_layout()`
  returns structured layout only when the structured entry is valid and either
  no legacy shadow exists or parity was checked and matches; on structured/text
  mismatch it silently falls through to `compute_aggregate_type_layout(trimmed,
  type_decls)`, making legacy text authoritative again. In
  `memory/addressing.cpp`, `resolve_aggregate_byte_offset_projection()` and
  `resolve_aggregate_child_index_projection()` call
  `compute_aggregate_type_layout()` directly for parent and child layouts, so
  projection helpers remain text-authority even when the caller has structured
  layout data. `StructuredTypeSpellingContext` is display-only and should not be
  promoted to semantic layout authority.

## Suggested Next

First implementation packet: make `lookup_backend_aggregate_type_layout()` stop
silently falling back to legacy `TypeDeclMap` when a structured declaration is
present but its parity check against the legacy shadow mismatches. Keep
structured-missing fallback intact, keep matched structured-present behavior
structured-primary, and add focused coverage in `backend_prepare_structured_context`
or `backend_lir_to_bir_notes` that corrupts the legacy shadow while structured
layout is present.

Supervisor-ready proof command for that packet:
`cmake --build build --target backend_prepare_structured_context backend_lir_to_bir_notes && ctest --test-dir build -R '^(backend_prepare_structured_context|backend_lir_to_bir_notes)$' --output-on-failure`

## Watchouts

Do not delete `LirModule::type_decls`; the fallback route is still needed when
`struct_decls` is absent. Do not use `StructuredTypeSpellingContext` as a
semantic layout table; it intentionally carries final spelling metadata for BIR
printing. The first packet should avoid expectation downgrades and should prove
same-feature behavior: structured-present match, structured-present mismatch,
and structured-missing fallback.

## Proof

No build or test required for this trace-only packet. Proof run:
`git diff --check -- todo.md`.
