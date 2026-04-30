# Current Packet

Status: Active
Source Idea Path: ideas/open/138_lir_bir_backend_aggregate_layout_type_decl_text_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Consolidate Fallback and Mismatch Reporting

## Just Finished

Step 5 fallback/mismatch API trace completed for idea 138. The central
structured layout table already records `structured_layout`, `legacy_layout`,
`legacy_found`, `structured_found`, and parity state in
`BackendStructuredLayoutEntry`, and module lowering reports parity mismatches
through `report_backend_structured_layout_parity_notes()`. The gap is that
`lookup_backend_aggregate_type_layout()` returns only `AggregateTypeLayout`, so
callers can observe the selected shape but not whether it was
structured-primary, legacy fallback, or a structured/text mismatch case.

## Suggested Next

Smallest first Step 5 implementation packet:

Introduce a narrow lookup-result wrapper for the central backend aggregate
layout helper, for example `BackendAggregateLayoutLookup`, containing the
selected `AggregateTypeLayout` plus source/status facts such as
structured-primary, legacy fallback used, and structured/text mismatch visible.
Add a compatibility accessor or wrapper so existing layout consumers can keep
using the selected layout while focused tests assert the new status for:
matching structured table, mismatched structured table, and empty-table legacy
fallback. Keep the first packet in `src/backend/bir/lir_to_bir/lowering.hpp`,
`src/backend/bir/lir_to_bir/types.cpp`, and
`tests/backend/backend_prepare_structured_context_test.cpp`; do not rewrite the
memory/global/aggregate call sites yet.

Exact proof command for that packet:
`cmake --build build --target backend_prepare_structured_context_test > test_after.log 2>&1 && ctest --test-dir build -R '^backend_prepare_structured_context$' --output-on-failure >> test_after.log 2>&1`.

## Watchouts

Structured-primary lookup itself is centralized in `types.cpp`, but the
optional-table wrapper pattern is duplicated in `aggregate.cpp`,
`global_initializers.cpp`, `globals.cpp`, `memory/addressing.cpp`,
`memory/local_gep.cpp`, and `memory/local_slots.cpp`. Those wrappers are a good
later cleanup only after the central lookup result shape exists.

Mismatch reporting is currently module-level and table-wide: module lowering
builds `BackendStructuredLayoutTable`, calls
`report_backend_structured_layout_parity_notes()`, then passes the table into
global and function lowering. That keeps mismatches visible for real module
lowering, but direct helper users and projection tests can only infer mismatch
behavior from returned layout shape. Preserve raw/hand-built LIR compatibility:
when the structured table is empty or lacks the `%struct.*` key,
`compute_aggregate_type_layout()` must remain the legacy `TypeDeclMap` fallback.

## Proof

Proof for this Step 5 trace packet:
`git diff --check -- todo.md`.

This delegated packet is docs/state only and does not produce `test_after.log`.
