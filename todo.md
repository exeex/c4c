Status: Active
Source Idea Path: ideas/open/135_sema_structured_owner_static_member_lookup_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Make Enclosing Method Owner Lookup Structured-Primary

# Current Packet

## Just Finished

Plan Step 3 `Make Enclosing Method Owner Lookup Structured-Primary` completed
for method-body owner and implicit field validation.

`enclosing_method_owner_record` remains the structured owner authority for
`validate_function`; the rendered string helper was renamed to
`enclosing_method_owner_struct_compatibility` so legacy string fallback use is
explicit. Method validation continues to seed `current_method_struct_key_` from
the resolved owner record when one is available.

Implicit out-of-class method field lookup now consults
`current_method_struct_key_` first when the member reference carries a
structured `TextId`, accepting structured hits before falling back to the
legacy rendered field-name path for source cases that do not yet have complete
structured member coverage.

Added `test_sema_method_validation_prefers_structured_owner_key_for_fields` to
`frontend_parser_tests`. The fixture gives an out-of-class method a misleading
rendered owner spelling (`wrong::Owner::get`) and a misleading rendered field
spelling (`stale`) while the structured owner/member keys point at
`real::Owner::actual`; validation succeeds only when the structured owner/member
identity is used.

## Suggested Next

Next coherent packet: continue with the next supervisor-selected owner cleanup
slice, likely the downstream HIR or lowering paths that still consume rendered
owner strings after sema has selected a structured owner record.

## Watchouts

`method_owner_key_from_qualifier` deliberately refuses to build a key if the
last qualifier segment does not name the parsed owner. This preserves the
existing compatibility path for parsed operator/function shapes where qualifier
metadata is present but is not an owner carrier.

The structured implicit-field path still falls back to legacy rendered field
lookup on structured misses because existing source positive cases expose
incomplete structured member coverage for some parsed method bodies. The new
structured-hit path covers the misleading spelling case without making
structured misses fatal.

`enclosing_method_owner_struct_compatibility` still returns rendered strings for
legacy callers; the structured record path is carried separately through
`enclosing_method_owner_record` and `current_method_struct_key_`.

## Proof

Ran the supervisor-selected proof exactly:

`cmake --build build --target c4cll frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_positive_sema_namespaced_out_of_class_method_context_frontend_cpp|cpp_positive_sema_qualified_namespaced_out_of_class_method_context_frontend_cpp|cpp_positive_sema_out_of_class_member_owner_scope_parse_cpp|cpp_positive_sema_operator_this_out_of_class_runtime_cpp|cpp_positive_sema_operator_conversion_out_of_class_parse_cpp|cpp_positive_sema_inherited_implicit_member_out_of_class_runtime_cpp|cpp_positive_sema_using_namespace_struct_method_runtime_cpp|cpp_positive_sema_template_member_owner_resolution_cpp|cpp_positive_sema_namespace_struct_collision_runtime_cpp|cpp_positive_sema_namespace_template_struct_basic_cpp)$' >> test_after.log 2>&1`

Result: build succeeded and CTest reported 11/11 passing. Proof log:
`test_after.log`.
