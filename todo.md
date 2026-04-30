# Current Packet

Status: Active
Source Idea Path: ideas/open/138_lir_bir_backend_aggregate_layout_type_decl_text_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Consolidate Fallback and Mismatch Reporting

## Just Finished

Step 4 global initializer layout coverage completed for idea 138. Existing
global lowering already passes `BackendStructuredLayoutTable` through
`lower_minimal_global()` into `lower_aggregate_initializer()`; focused coverage
now proves the real global lowering route uses structured i32 layout data when
a mismatched legacy `%struct.Pair` shadow exists, emits the structured-vs-legacy
parity note, and preserves the legacy i64 `TypeDeclMap` fallback when no
structured declaration exists.

## Suggested Next

Execute Step 5 by consolidating aggregate layout fallback and mismatch reporting
around the shared lookup/text-parsing helpers. Start from the Step 4 finding
that global lowering already routes structured layout data through
`BackendStructuredLayoutTable` into `lower_aggregate_initializer()`, while the
empty-table case must keep the legacy `TypeDeclMap` fallback for raw or
hand-built LIR.

Suggested first packet:

- Inspect the aggregate layout lookup helper APIs and callers for duplicated
  fallback or mismatch handling.
- Tighten names or result shapes so callers can distinguish
  structured-primary, fallback-used, and structured/text mismatch cases without
  reparsing final `%type` text.
- Preserve final emitted text, diagnostics, and dumps as display/emission
  concerns only.
- Prove focused mismatch/fallback visibility, then run the broader backend/LIR
  checkpoint selected by the supervisor.

## Watchouts

Step 4 required no production change. `global_initializers.cpp` still needs the
legacy fallback path because module lowering always supplies a structured layout
table, but that table is empty when `struct_decls` is absent and
`lookup_backend_aggregate_type_layout()` then falls back to `TypeDeclMap`.

## Proof

Proof passed for this Step 4 packet:
`cmake --build build --target backend_prepare_structured_context_test backend_lir_to_bir_notes_test > test_after.log 2>&1 && ctest --test-dir build -R '^(backend_prepare_structured_context|backend_lir_to_bir_notes)$' --output-on-failure >> test_after.log 2>&1`.

Proof log: `test_after.log`.
