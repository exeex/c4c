Status: Active
Source Idea Path: ideas/open/136_hir_structured_record_template_lookup_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Validation and Acceptance

# Current Packet

## Just Finished

Completed Plan Step 5 `Make Struct Method and Member-Symbol Maps Structured-Primary`.
Step 5 code slice is committed as `7ef61ccf`.

Member-symbol lookup now prefers `HirStructMemberLookupKey` owner/member identity
when a structured carrier is available, including direct key lookup and
`TypeSpec.record_def` plus member `TextId` routes from owned member-expression
callers. Rendered `module_->member_symbols` lookup remains the compatibility
fallback and parity source. Added focused HIR coverage for stale rendered owner
and member spelling losing to structured member-symbol authority.

Struct method and static-member maps already satisfied this Step 5 classification
from prior packets: method mangled/link-name/return-type lookup and static
member decl/const lookup already try owner-key maps first and retain rendered
maps as explicit fallback/parity bridges.

## Suggested Next

Next coherent packet: execute Plan Step 6 `Validation and Acceptance`.

Validation should prove the structured-primary HIR lookup authority across the
completed Step 1-5 families: namespace-qualified records, out-of-class methods,
scoped static members, template lookup, and member-symbol lookup. Record exact
commands and results in this file and `test_after.log`.

## Watchouts

Member-symbol callers outside the owned files may still use the rendered string
bridge, but that bridge now also prefers owner keys when `module_->struct_def_owner_index`
can derive a complete `HirStructMemberLookupKey`. Direct `TypeSpec` routing
currently uses `record_def`; template-origin-only carriers still depend on the
resolved rendered owner tag fallback unless another packet adds a direct
template-instance member key carrier.

## Proof

Latest completed implementation proof from Step 5:

Ran the supervisor-selected proof exactly:
`cmake --build build --target c4cll frontend_hir_tests frontend_hir_lookup_tests frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|frontend_hir_lookup_tests|frontend_parser_tests|cpp_hir_template_member_owner_resolution|cpp_hir_template_member_owner_chain|cpp_hir_template_member_owner_decl_and_cast|cpp_hir_template_alias_member_owner|cpp_hir_template_member_owner_field_and_local|cpp_hir_template_member_owner_signature_local|cpp_hir_expr_call_member_helper|cpp_hir_expr_operator_member_helper|cpp_hir_expr_object_materialization_helper|cpp_positive_sema_template_member_owner_resolution_cpp|cpp_positive_sema_template_struct_advanced_cpp|cpp_positive_sema_template_struct_nested_cpp|cpp_positive_sema_inherited_base_member_access_runtime_cpp|cpp_positive_sema_operator_deref_member_basic_cpp|cpp_positive_sema_operator_arrow_member_basic_cpp|cpp_positive_sema_operator_subscript_member_basic_cpp|cpp_positive_sema_inherited_static_member_lookup_runtime_cpp|cpp_positive_sema_unqualified_static_member_call_runtime_cpp)$' >> test_after.log 2>&1`

Result: passed, 21/21 tests green. Proof log: `test_after.log`.
