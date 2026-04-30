# Current Packet

Status: Active
Source Idea Path: ideas/open/138_lir_bir_backend_aggregate_layout_type_decl_text_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Consolidate Fallback and Mismatch Reporting

## Just Finished

Step 5 local GEP optional-table wrapper packet completed for idea 138. Migrated
the narrow `memory/local_gep.cpp` layout wrapper family to
`lookup_backend_aggregate_type_layout_result()` by adding
`lookup_local_gep_layout_result()` and keeping `lookup_local_gep_layout()` as
the selected-layout compatibility wrapper. The no-structured-table path still
uses legacy `compute_aggregate_type_layout()` fallback result metadata, while
the structured-table path now preserves central selected-layout,
legacy-fallback, and mismatch reporting behavior.

## Suggested Next

Smallest next Step 5 packet: migrate the analogous optional-table wrapper
family in `memory/addressing.cpp` to the central lookup-result API, then decide
whether Step 5 needs another narrow packet for remaining memory/global helper
wrappers.

## Watchouts

This packet intentionally did not rewrite local GEP call sites to inspect the
new result flags; selected-layout behavior remains unchanged through the
compatibility wrapper. `memory/addressing.cpp`, `memory/local_slots.cpp`,
`global_initializers.cpp`, and `globals.cpp` still have optional-table wrappers
or direct selected-layout calls that can be migrated in separate narrow packets.

## Proof

Proof for this Step 5 local GEP wrapper packet:
`cmake --build build --target backend_prepare_structured_context_test backend_lir_to_bir_notes_test > test_after.log 2>&1 && ctest --test-dir build -R '^(backend_prepare_structured_context|backend_lir_to_bir_notes)$' --output-on-failure >> test_after.log 2>&1`.

Result: passed. Proof log: `test_after.log`.
