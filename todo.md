# Current Packet

Status: Active
Source Idea Path: ideas/open/138_lir_bir_backend_aggregate_layout_type_decl_text_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Move Aggregate Initializer Layout Consumers Off Text Authority

## Just Finished

Step 3 aggregate initializer layout coverage completed for idea 138. Existing
`lower_aggregate_initializer()` structured overloads already pass
`BackendStructuredLayoutTable` through recursive aggregate and zero-fill
lowering; focused coverage now proves a structured-present mismatched legacy
shadow uses the structured i32 layout, while a structured-missing table keeps
the legacy i64 `TypeDeclMap` fallback.

## Suggested Next

Next coherent packet: execute Step 4 by making global initializer layout
coverage prove structured-present behavior, structured-missing fallback, and
mismatch visibility through the global lowering route that calls
`lower_aggregate_initializer()`.

Suggested proof for that packet:
`cmake --build build --target backend_prepare_structured_context_test backend_lir_to_bir_notes_test > test_after.log 2>&1 && ctest --test-dir build -R '^(backend_prepare_structured_context|backend_lir_to_bir_notes)$' --output-on-failure >> test_after.log 2>&1`.

## Watchouts

Step 3 required no production change: `global_initializers.cpp` already uses
the structured-aware `lookup_global_initializer_layout()` path when the caller
supplies structured layouts. Do not delete `LirModule::type_decls`; the
fallback route is still needed when `struct_decls` is absent.

## Proof

Proof passed for this Step 3 packet:
`cmake --build build --target backend_prepare_structured_context_test backend_lir_to_bir_notes_test > test_after.log 2>&1 && ctest --test-dir build -R '^(backend_prepare_structured_context|backend_lir_to_bir_notes)$' --output-on-failure >> test_after.log 2>&1`.

Proof log: `test_after.log`.
