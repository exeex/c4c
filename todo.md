# Current Packet

Status: Active
Source Idea Path: ideas/open/138_lir_bir_backend_aggregate_layout_type_decl_text_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Make Backend Layout Lookup Structured-Primary

## Just Finished

Step 2 projection-helper cleanup completed for idea 138. The aggregate
byte-offset and child-index projection helpers in `memory/addressing.cpp` now
route parent and child layout resolution through the structured-aware lookup
path, with the no-structured overload preserving legacy fallback through an
empty structured table. Focused coverage in
`backend_prepare_structured_context` now proves structured-present projection
behavior with mismatched legacy shadow text and structured-missing fallback for
both byte-offset and child-index projections.

## Suggested Next

Next coherent packet: continue Step 2 by scanning the remaining aggregate layout
consumers in `memory/addressing.cpp` for any direct legacy fallback routes that
should prefer structured layout data when the caller has a structured table.

## Watchouts

Do not delete `LirModule::type_decls`; the fallback route is still needed when
`struct_decls` is absent. Do not use `StructuredTypeSpellingContext` as a
semantic layout table; it intentionally carries final spelling metadata for BIR
printing. `lookup_addressing_layout()` still owns the actual structured-present
versus structured-missing decision for memory addressing helpers.

## Proof

Proof passed and was written to `test_after.log`:
`cmake --build build --target backend_prepare_structured_context_test backend_lir_to_bir_notes_test > test_after.log 2>&1 && ctest --test-dir build -R '^(backend_prepare_structured_context|backend_lir_to_bir_notes)$' --output-on-failure >> test_after.log 2>&1`.
