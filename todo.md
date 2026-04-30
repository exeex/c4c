# Current Packet

Status: Active
Source Idea Path: ideas/open/138_lir_bir_backend_aggregate_layout_type_decl_text_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Consolidate Fallback and Mismatch Reporting

## Just Finished

Step 5 review finding fixed for `BackendAggregateLayoutLookup` fallback status.
`lookup_backend_aggregate_type_layout_result()` now reports
`used_legacy_fallback` only when the non-structured path resolves a named
`%type` through the legacy `TypeDeclMap`; scalar, inline aggregate, invalid,
and unresolved named layouts no longer over-report fallback use. Focused
coverage was added to preserve existing structured/mismatch/fallback behavior
and assert the corrected scalar/inline/invalid/missing status cases.

## Suggested Next

Supervisor can review and commit this Step 5 correction slice, then decide
whether any remaining optional-table wrapper callers need the same status
semantics in a separate bounded packet.

## Watchouts

This packet intentionally did not touch `global_initializers.cpp`, `globals.cpp`,
memory wrappers, or `aggregate.cpp`; the owned fix is the shared
`BackendAggregateLayoutLookup` helper plus focused status coverage.

## Proof

Proof for this Step 5 review correction:
`cmake --build build --target backend_prepare_structured_context_test > test_after.log 2>&1 && ctest --test-dir build -R '^backend_prepare_structured_context$' --output-on-failure >> test_after.log 2>&1`.

Result: passed. Proof log: `test_after.log`.
