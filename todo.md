Status: Active
Source Idea Path: ideas/open/118_lir_bir_legacy_type_text_removal.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Remove Covered Layout Authority From Legacy Declarations

# Current Packet

## Just Finished

Completed `plan.md` Step 2 for the covered named structured aggregate layout
path used by aggregate call ABI lowering. `lookup_backend_aggregate_type_layout`
now returns valid structured layout metadata for a named type when no legacy
`TypeDeclMap` declaration body exists, and still only trusts structured layout
over legacy text when parity was checked and matched.

Strengthened `backend_prepare_structured_context` so the real `%struct.Pair`
aggregate-call fixture has structured declaration metadata but no
`module.type_decls` body, then still lowers and prints the expected sret
call ABI shape through structured layout authority.

## Suggested Next

Delegate the next narrow covered layout-authority packet only after choosing a
remaining `TypeDeclMap` fallback surface whose structured metadata is already
complete enough to replace legacy declaration text without weakening parity
mismatch diagnostics or fallback behavior.

## Watchouts

- If a legacy declaration exists and parity is checked but mismatched,
  `lookup_backend_aggregate_type_layout` still falls back to
  `compute_aggregate_type_layout(trimmed, type_decls)` so compatibility and
  parity notes are preserved.
- Structured layout is only selected when the structured layout itself is valid;
  invalid or absent structured entries still use existing `TypeDeclMap`
  fallback behavior.
- The context-population parity fixture remains intentionally backed by both
  structured declarations and legacy `type_decls`.

## Remaining Legacy Surface Inventory

- `LirModule::type_decls` and `TypeDeclMap` remain active layout/lowering
  fallback authority for unnamed aggregate text, named types without valid
  structured layout metadata, invalid structured layouts, and named structured
  types with legacy parity mismatches; they also remain proof-only shadow data
  for structured-layout parity notes.
- Raw `LirTypeRef` text remains blocked type-ref authority for call return
  parsing, typed call args, GEP/load/store/cast/phi/select type strings, and
  aggregate field text, and remains final spelling/output data for LIR printing
  and final LLVM emission.
- Function `signature_text`, extern `return_type_str`, and global `llvm_type`
  strings remain active lowering authority and final spelling/output data for
  their current declaration, ABI, global storage, initializer, and emission
  paths.
- `CallInst::return_type_name` still covers scalar/pointer returns, runtime
  intrinsics, inline asm, prepared-BIR/manual calls, calls without structured
  return context, and structured aggregate sret calls lacking
  `structured_return_type_name`.

## Proof

Ran `{ cmake --build build-backend --target backend_prepare_structured_context_test backend_lir_to_bir_notes_test && ctest --test-dir build-backend -j --output-on-failure -R '^(backend_prepare_structured_context|backend_lir_to_bir_notes)$'; } > test_after.log 2>&1`;
passed. Proof log: `test_after.log`.
