Status: Active
Source Idea Path: ideas/open/135_sema_structured_owner_static_member_lookup_cleanup.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Tighten Unqualified Name Fallbacks

# Current Packet

## Just Finished

Plan Step 5 `Tighten Unqualified Name Fallbacks` completed for sema
unqualified variable/function/enum symbol lookup.

`lookup_symbol` now names rendered `Node::name` hits as compatibility results
and returns structured local, global, and enum key hits first when the reference
node carries usable structured identity. `lookup_function_by_name` likewise
returns a structured function key hit before falling back to the rendered-name
compatibility map.

The `NK_VAR` rendered `unqualified_name` retry is now documented as a narrow
compatibility path for producers that still render unqualified ids as visible
namespace spellings before local binding is available.

Added `test_sema_unqualified_symbol_lookup_prefers_structured_key_over_rendered_spelling`
to `frontend_parser_tests`. The fixture gives an unqualified local parameter
reference a misleading rendered full spelling (`wrong::stale`) and a misleading
rendered unqualified spelling (`stale`) while its `unqualified_text_id` points
at the real parameter `actual`; validation succeeds only through the structured
local name key.

## Suggested Next

Next coherent packet: continue with the next supervisor-selected cleanup slice,
likely auditing remaining rendered-name compatibility paths that still return
legacy results before structured keys in sema or downstream HIR/lowering code.

## Watchouts

This packet intentionally keeps rendered-name compatibility fallbacks alive
when a reference lacks structured identity or the structured lookup misses.
The remaining fallback is explicitly named but not made fatal because existing
producer paths can still arrive without complete structured carriers.

The focused new test covers the local `sema_local_name_key` path. The production
change also makes symbol-key lookups for globals/functions/enums structured
primary when `sema_symbol_name_key` is available.

## Proof

Ran the supervisor-selected proof exactly:

`cmake --build build --target c4cll frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_positive_sema_unqualified_static_member_call_runtime_cpp|cpp_positive_sema_using_namespace_fn_lookup_cpp|cpp_positive_sema_using_namespace_struct_method_runtime_cpp|cpp_positive_sema_scope_shadowing_block_runtime_cpp|cpp_positive_sema_scope_shadowing_param_runtime_cpp|cpp_positive_sema_scope_resolution_expr_parse_cpp|cpp_positive_sema_operator_this_out_of_class_runtime_cpp|cpp_positive_sema_inherited_implicit_member_out_of_class_runtime_cpp|cpp_negative_tests_bad_scope_for_init_leak|cpp_negative_tests_bad_scope_inner_block_leak|negative_tests_bad_scope_out_of_scope|positive_sema_ok_enum_scope_local_over_global_c|positive_sema_ok_enum_scope_no_leak_after_block_c)$' >> test_after.log 2>&1`

Result: build succeeded and CTest reported 14/14 passing. Proof log:
`test_after.log`.
