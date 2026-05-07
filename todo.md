Status: Active
Source Idea Path: ideas/open/146_qualified_name_deferred_carrier_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Define Structured Deferred Lookup Carrier For Dependent Cases

# Current Packet

## Just Finished

Step 5: Define Structured Deferred Lookup Carrier For Dependent Cases added an explicit deferred member-type owner key beside the deferred member TextId, populated it from parser dependent/member typedef construction paths, propagated and cleared it through Sema/HIR type handling, and made HIR member typedef resolution prefer member TextId metadata over display spelling when resolving pending deferred member types.

## Suggested Next

Supervisor can review and commit this Step 5 structured deferred carrier slice, then choose whether to move into Step 6 HIR structural consumption cleanup.

## Watchouts

The new carrier is intentionally minimal: deferred member type owner `QualifiedNameKey` plus member `TextId`; legacy `deferred_member_type_name` remains for display/diagnostic compatibility and fallback paths. Parser-owned `QualifiedNameKey::qualifier_path_id` is preserved where available, while HIR still consumes the owner base/context/global fields and existing `TypeSpec` qualifier TextIds.

## Proof

`cd /workspaces/c4c && { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_lookup_tests|cpp_hir_parser_type_base_deferred_member_template_origin_structured_metadata|cpp_hir_parser_type_base_pending_base_substitution_structured_metadata|cpp_hir_parser_type_base_instantiated_deferred_member_structured_metadata|cpp_hir_parser_declarator_deferred_owner_structured_metadata|cpp_positive_sema_dependent_template_typename_member_parse_cpp|cpp_positive_sema_ctor_init_dependent_typename_index_tuple_parse_cpp|cpp_positive_sema_c_style_cast_namespace_qualified_dependent_template_member_fn_ptr_ref_qual_parse_cpp|cpp_positive_sema_c_style_cast_global_qualified_dependent_template_member_fn_ptr_ref_qual_parse_cpp)$'; } > test_after.log 2>&1`

Passed. `test_after.log` is the canonical executor proof log. CTest matched and ran all 9 delegated tests; all passed after the build completed.
