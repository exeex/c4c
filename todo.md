# Current Packet

Status: Active
Source Idea Path: ideas/open/138_lir_bir_backend_aggregate_layout_type_decl_text_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Consolidate Fallback and Mismatch Reporting

## Just Finished

Step 5 wrapper consolidation completed for the global initializer/global
optional-table layout family. `global_initializers.cpp` now exposes
`lookup_global_initializer_layout_result()` and `globals.cpp` now exposes
`lookup_global_layout_result()`, both returning
`BackendAggregateLayoutLookup` from `lookup_backend_aggregate_type_layout_result()`
when a structured table is present and preserving explicit no-structured-table
legacy fallback metadata. The existing selected-layout helpers remain as
compatibility `.layout` adapters.

## Suggested Next

Pause for supervisor review of Step 5 before any further implementation packet,
unless the supervisor identifies a current blocker that needs a bounded executor
follow-up.

## Watchouts

No focused tests were added; the change is a helper-surface consolidation and
the existing compatibility wrappers preserve current selected-layout behavior.
`aggregate.cpp` direct selected-layout calls remain outside this packet by
delegation boundary.

## Proof

Proof for this Step 5 wrapper consolidation:
`cmake --build build --target backend_prepare_structured_context_test backend_lir_to_bir_notes_test > test_after.log 2>&1 && ctest --test-dir build -R '^(backend_prepare_structured_context|backend_lir_to_bir_notes)$' --output-on-failure >> test_after.log 2>&1`.

Result: passed. Proof log: `test_after.log`.
