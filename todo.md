# Current Packet

Status: Active
Source Idea Path: ideas/open/138_lir_bir_backend_aggregate_layout_type_decl_text_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Consolidate Fallback and Mismatch Reporting

## Just Finished

Step 5 wrapper/reporting scan completed for idea 138. The remaining narrow
optional-table wrapper surface that should be handled before supervisor review
is in `global_initializers.cpp` and `globals.cpp`: `lookup_global_initializer_layout()`
and `lookup_global_layout()` still return only `AggregateTypeLayout`, dispatching
between `lookup_backend_aggregate_type_layout()` and
`compute_aggregate_type_layout()` and hiding the central selected-layout,
legacy-fallback, and mismatch flags now exposed by
`lookup_backend_aggregate_type_layout_result()`.

## Suggested Next

Smallest next Step 5 implementation packet: migrate the global initializer/global
layout wrapper family in `global_initializers.cpp` and `globals.cpp` to
result-returning helpers, e.g. add `lookup_global_initializer_layout_result()`
and `lookup_global_layout_result()` that preserve explicit no-structured-table
legacy fallback metadata, then keep the existing selected-layout wrappers as
compatibility `.layout` adapters. Exact proof command:
`cmake --build build --target backend_prepare_structured_context_test backend_lir_to_bir_notes_test > test_after.log 2>&1 && ctest --test-dir build -R '^(backend_prepare_structured_context|backend_lir_to_bir_notes)$' --output-on-failure >> test_after.log 2>&1`.

## Watchouts

This scan did not touch implementation files. `aggregate.cpp` still has direct
selected-layout calls, but the clearest remaining wrapper/reporting packet is
the global initializer/global optional-table family because those helpers have
the same hidden fallback/mismatch shape as the already-migrated memory wrappers.
After that packet, pause for supervisor reviewer/plan-owner review unless the
reviewer asks for a separate aggregate/byval cleanup.

## Proof

Proof for this Step 5 wrapper/reporting scan:
`git diff --check -- todo.md`.

Result: passed. This scan-only packet did not write a proof log.
