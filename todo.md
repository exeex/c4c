# Current Packet

Status: Active
Source Idea Path: ideas/open/138_lir_bir_backend_aggregate_layout_type_decl_text_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Move Global Initializer Layout Consumers Off Text Authority

## Just Finished

Step 4 global initializer layout coverage completed for idea 138. Existing
global lowering already passes `BackendStructuredLayoutTable` through
`lower_minimal_global()` into `lower_aggregate_initializer()`; focused coverage
now proves the real global lowering route uses structured i32 layout data when
a mismatched legacy `%struct.Pair` shadow exists, emits the structured-vs-legacy
parity note, and preserves the legacy i64 `TypeDeclMap` fallback when no
structured declaration exists.

## Suggested Next

Next packet should move to the next supervisor-selected idea 138 consumer or ask
plan-owner/reviewer whether Step 4 is exhausted, since no production code change
was needed for the global initializer route.

## Watchouts

Step 4 required no production change. `global_initializers.cpp` still needs the
legacy fallback path because module lowering always supplies a structured layout
table, but that table is empty when `struct_decls` is absent and
`lookup_backend_aggregate_type_layout()` then falls back to `TypeDeclMap`.

## Proof

Proof passed for this Step 4 packet:
`cmake --build build --target backend_prepare_structured_context_test backend_lir_to_bir_notes_test > test_after.log 2>&1 && ctest --test-dir build -R '^(backend_prepare_structured_context|backend_lir_to_bir_notes)$' --output-on-failure >> test_after.log 2>&1`.

Proof log: `test_after.log`.
