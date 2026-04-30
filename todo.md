# Current Packet

Status: Active
Source Idea Path: ideas/open/138_lir_bir_backend_aggregate_layout_type_decl_text_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Make Backend Layout Lookup Structured-Primary

## Just Finished

Step 2 implementation completed for idea 138. `lookup_backend_aggregate_type_layout()`
now returns a valid structured layout whenever a structured entry is present,
including when the legacy `TypeDeclMap` shadow exists but parity mismatches.
Structured-missing fallback still routes through `compute_aggregate_type_layout()`.
Focused coverage in `backend_prepare_structured_context` now proves
structured-present match, structured-present mismatch with corrupted legacy
shadow text, and structured-missing fallback.

## Suggested Next

Next coherent packet: continue Step 2 by routing remaining aggregate projection
helpers in `memory/addressing.cpp` away from direct legacy
`compute_aggregate_type_layout()` calls when structured layout data is available.

## Watchouts

Do not delete `LirModule::type_decls`; the fallback route is still needed when
`struct_decls` is absent. Do not use `StructuredTypeSpellingContext` as a
semantic layout table; it intentionally carries final spelling metadata for BIR
printing. Step 2's top-level lookup now treats valid structured entries as
authoritative, but `memory/addressing.cpp` still has same-feature projection
paths that need structured-primary cleanup in a separate packet.

## Proof

Proof passed and was written to `test_after.log`:
`cmake --build build --target backend_prepare_structured_context_test backend_lir_to_bir_notes_test > test_after.log 2>&1 && ctest --test-dir build -R '^(backend_prepare_structured_context|backend_lir_to_bir_notes)$' --output-on-failure >> test_after.log 2>&1`.
