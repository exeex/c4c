# Current Packet

Status: Active
Source Idea Path: ideas/open/138_lir_bir_backend_aggregate_layout_type_decl_text_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Consolidate Fallback and Mismatch Reporting

## Just Finished

Step 5 review findings addressed for aggregate selected-layout and no-structured
optional-table fallback status. `aggregate.cpp` selected-layout calls now route
through the status-aware central lookup when structured layouts are available
while preserving `.layout` behavior. Optional-table wrapper no-structured paths
now use the central lookup with an empty structured table, so scalar, inline,
invalid, and unresolved named layouts no longer over-report legacy fallback.

## Suggested Next

Supervisor can review this aggregate/status packet and decide whether Step 5 is
ready for lifecycle review or commit.

## Watchouts

No focused tests were added in this packet; existing structured-context status
coverage plus the backend notes subset covered the corrected lookup semantics
and affected lowering paths.

## Proof

Proof for this Step 5 aggregate/status packet:
`cmake --build build --target backend_prepare_structured_context_test backend_lir_to_bir_notes_test > test_after.log 2>&1 && ctest --test-dir build -R '^(backend_prepare_structured_context|backend_lir_to_bir_notes)$' --output-on-failure >> test_after.log 2>&1`.

Result: passed. Proof log: `test_after.log`.
