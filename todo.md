Status: Active
Source Idea Path: ideas/open/136_hir_structured_record_template_lookup_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Make Template Struct Lookup Structured-Primary

# Current Packet

## Just Finished

Completed Plan Step 4 `Make Template Struct Lookup Structured-Primary`.

Template struct primary lookup now has a declaration-aware path that prefers
`HirRecordOwnerKey`/declaration identity and falls back to rendered names only
when no structured producer data resolves. Specialization lookup now checks the
Lowerer owner-key map and compile-time-state owner-key lookup before rendered
primary-name compatibility maps. Pending template-type canonicalization also
uses `TypeSpec.record_def` when present. Added focused HIR tests covering stale
rendered primary names and specialization owners.

## Suggested Next

Next coherent packet: continue with the supervisor-selected follow-up for
structured HIR record/template lookup authority cleanup, or review whether this
Step 4 slice is ready to commit.

## Watchouts

Rendered template primary/specialization maps remain compatibility fallback for
callers without declaration or owner identity. The new structured route relies
on producer-supplied `record_def`, `unqualified_name`, or compile-time registry
text identity being available.

## Proof

Ran the supervisor-selected proof exactly:
`cmake --build build --target c4cll frontend_hir_tests frontend_hir_lookup_tests frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|frontend_hir_lookup_tests|frontend_parser_tests|cpp_hir_template_origin|cpp_hir_template_struct_registry_primary_only|cpp_hir_template_member_owner_resolution|cpp_hir_template_struct_field_array_extent|cpp_hir_template_struct_inherited_method_binding|cpp_hir_template_member_owner_chain|cpp_hir_template_member_owner_decl_and_cast|cpp_hir_template_member_owner_field_and_local|cpp_hir_template_member_owner_signature_local|cpp_hir_template_struct_body_instantiation|cpp_hir_template_struct_arg_materialization|cpp_positive_sema_namespace_template_struct_basic_cpp|cpp_positive_sema_template_struct_advanced_cpp|cpp_positive_sema_template_struct_nested_cpp|cpp_positive_sema_template_struct_specialization_runtime_cpp|cpp_positive_sema_template_member_owner_resolution_cpp)$' >> test_after.log 2>&1`

Result: passed, 19/19 tests green. Proof log: `test_after.log`.
