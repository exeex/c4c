# Current Packet

Status: Active
Source Idea Path: ideas/open/138_lir_bir_backend_aggregate_layout_type_decl_text_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Consolidate Fallback and Mismatch Reporting

## Just Finished

Step 5 first consolidation packet completed for idea 138. Added
`BackendAggregateLayoutLookup` plus
`lookup_backend_aggregate_type_layout_result()` so central backend aggregate
layout lookup now exposes the selected layout, structured-layout use,
legacy-fallback use, and structured/text mismatch status. Kept
`lookup_backend_aggregate_type_layout()` as the compatibility selected-layout
wrapper for existing consumers, and added focused structured-context assertions
for matching structured lookup, mismatched structured lookup, and empty-table
legacy fallback.

## Suggested Next

Smallest next Step 5 packet: migrate one duplicated optional-table wrapper
family to the central lookup-result API, preferably the narrow local helper in
`memory/local_gep.cpp` or `memory/addressing.cpp`, and preserve selected-layout
behavior while allowing callers to inspect source/status when needed.

## Watchouts

The first packet intentionally did not rewrite broad memory/global/aggregate
call sites. `used_legacy_fallback` is true when the central lookup selected the
legacy text path; `structured_text_mismatch` is reported only from a structured
table entry with parity checked and failing. Empty-table and missing-key
lookups still preserve raw/hand-built LIR compatibility through
`compute_aggregate_type_layout()`.

## Proof

Proof for this Step 5 first consolidation packet:
`cmake --build build --target backend_prepare_structured_context_test > test_after.log 2>&1 && ctest --test-dir build -R '^backend_prepare_structured_context$' --output-on-failure >> test_after.log 2>&1`.

Result: passed. Proof log: `test_after.log`.
