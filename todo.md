# Current Packet

Status: Active
Source Idea Path: ideas/open/138_lir_bir_backend_aggregate_layout_type_decl_text_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 6
Current Step Title: Final Coverage and Regression Check

## Just Finished

Step 5 fallback and mismatch reporting consolidation completed for idea 138.
`BackendAggregateLayoutLookup` now exposes selected layout, structured-layout
use, legacy `%type` fallback use, and structured/text mismatch status.
`aggregate.cpp`, memory wrappers, and global initializer/global wrappers now
route through the status-aware central lookup while preserving selected-layout
compatibility adapters. No-structured optional-table paths use the central empty
structured-table lookup, so scalar, inline, invalid, and unresolved named
layouts no longer over-report legacy fallback.

## Suggested Next

Execute Step 6 final coverage and regression check. Prove structured-present,
structured-missing fallback, mismatch, aggregate initializer, and global
initializer paths with focused tests, then run the supervisor-selected broader
validation or matching regression-guard scope. Inspect the diff for expectation
downgrades and testcase-overfit patterns before deciding whether the source
idea is close-ready or needs another runbook.

## Watchouts

`aggregate.cpp` selected-layout adapters still return `.layout` for existing
consumers by design; future diagnostics can use the result-bearing helpers
without reparsing final `%type` text. Do not remove legacy `type_decls`; raw and
hand-built LIR still need the explicit fallback.

## Proof

Focused proof passed during the aggregate/status packet:
`cmake --build build --target backend_prepare_structured_context_test backend_lir_to_bir_notes_test > test_after.log 2>&1 && ctest --test-dir build -R '^(backend_prepare_structured_context|backend_lir_to_bir_notes)$' --output-on-failure >> test_after.log 2>&1`.

Broader Step 5 backend/LIR checkpoint passed and was regression-guarded against
a matching `test_before.log`: `cmake --build build --target c4c_backend
frontend_lir_global_type_ref_test frontend_lir_function_signature_type_ref_test
frontend_lir_extern_decl_type_ref_test frontend_lir_call_type_ref_test
backend_lir_to_bir_notes_test backend_prepare_structured_context_test
backend_prepare_stack_layout_test backend_prepare_liveness_test
backend_prepared_printer_test > test_after.log 2>&1 && ctest --test-dir build
-j --output-on-failure -R
'^(frontend_lir_global_type_ref|frontend_lir_function_signature_type_ref|frontend_lir_extern_decl_type_ref|frontend_lir_call_type_ref|backend_lir_to_bir_notes|backend_prepare_structured_context|backend_prepare_stack_layout|backend_prepare_liveness|backend_prepared_printer|backend_cli_dump_bir_layout_sensitive_aggregate|backend_codegen_route_x86_64_(aggregate_return_pair|aggregate_param_return_pair|indirect_aggregate_param_return_pair|global_struct_array_read|global_struct_array_store|nested_global_struct_array_read|nested_global_struct_array_store|local_aggregate_member_offsets)_observe_semantic_bir)$'
>> test_after.log 2>&1`. Result: 18/18 passed before and after.
