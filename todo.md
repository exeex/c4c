Status: Active
Source Idea Path: ideas/open/146_qualified_name_deferred_carrier_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Separate Unqualified `TextId` APIs From Structured Qualified APIs

# Current Packet

## Just Finished

Step 2: Separate Unqualified `TextId` APIs From Structured Qualified APIs
completed the parser-side repair for dependent inherited
`Template<T>::type` base/member lookup. Qualified parameter types keep their
structured unresolved declaration fallback through
`QualifiedNameRef`/`TypeSpec` qualifier metadata, and dependent primary-template
owner/member forms now preserve a structured deferred member carrier instead
of relying on rendered compound `TextId` authority.

The record-base and unresolved-template recovery paths also now preserve
record/function bodies when template value expressions contain relational `<`
tokens, including functional-cast comparisons such as `T(-1) < T(0)`. This
keeps inherited alias-base/member lookup green while preserving the earlier
qualified parameter-list improvements.

## Suggested Next

Review and commit this Step 2 parser carrier slice, then choose the next
bounded qualified-name authority packet.

## Watchouts

Do not reintroduce rendered compound splitting in
`register_var_type_binding()` or known-function `TextId` helpers. Qualified
value/function authority should continue to enter through structured
`QualifiedNameKey` or context-plus-unqualified-`TextId` APIs.

The current partial repair intentionally keeps the unresolved qualified type
fallback out of `can_start_qualified_type_declaration()`, preserving the
frontend parser test that rejects rendered unresolved qualified type authority.

The new `Template<T>::type` carrier is structured through
`tpl_struct_origin_key`, `tpl_struct_args`, and
`deferred_member_type_text_id`; do not replace it with splitting or looking up
rendered qualified spellings. The base-clause skip changes are synchronization
boundary repairs, not semantic lookup authority.

## Proof

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|frontend_parser_tests|cpp_positive_sema_namespace_using_decl_typedef_and_value_frontend_cpp|cpp_positive_sema_qualified_namespaced_out_of_class_method_context_frontend_cpp|cpp_positive_sema_inherited_type_alias_base_member_lookup_parse_cpp|cpp_positive_sema_namespaced_inherited_type_alias_base_member_lookup_parse_cpp|cpp_eastl_integer_sequence_parse_recipe|cpp_eastl_type_traits_parse_recipe|cpp_eastl_utility_parse_recipe|cpp_eastl_vector_parse_recipe|cpp_eastl_memory_uses_allocator_parse_recipe|eastl_cpp_external_utility_frontend_basic_cpp)$'; } > test_after.log 2>&1`

Proof passed. `test_after.log` contains the canonical executor proof log:
12/12 selected tests passed, including the inherited alias-base/member lookup
cases, the previous qualified parameter-list coverage, and the delegated EASTL
regression recipes.
