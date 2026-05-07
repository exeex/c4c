Status: Active
Source Idea Path: ideas/open/146_qualified_name_deferred_carrier_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Separate Unqualified `TextId` APIs From Structured Qualified APIs

# Current Packet

## Just Finished

Step 2: Separate Unqualified `TextId` APIs From Structured Qualified APIs
completed the parser `using` value/function import registration repair.
Namespace-scope `using ns::Target;` value imports now register the imported
name through `register_structured_var_type_binding_in_context()` using the
importing context plus the unqualified `TextId`, instead of rendering
`wrap::Target` into a compound `TextId` and sending it through
`register_var_type_binding()`.

The nearby function import path was checked and remains on
`register_known_fn_name_in_context()`, which already uses the
context-plus-unqualified API and rejects compound rendered `TextId` authority.
Focused parser tests now cover both namespace value and function using imports.

## Suggested Next

Review and commit this Step 2 using-alias authority slice, then choose the next
bounded qualified-name authority packet.

## Watchouts

Do not reintroduce rendered compound splitting in
`register_var_type_binding()` or known-function `TextId` helpers. Qualified
value/function authority should continue to enter through structured
`QualifiedNameKey` or context-plus-unqualified-`TextId` APIs.

The using-declaration typedef import path still has its older rendered
compatibility mirror, but this packet only owned the value/function alias
registration path named by the supervisor.

## Proof

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|frontend_parser_tests|cpp_positive_sema_namespace_using_decl_typedef_and_value_frontend_cpp|cpp_positive_sema_qualified_namespaced_out_of_class_method_context_frontend_cpp|cpp_positive_sema_inherited_type_alias_base_member_lookup_parse_cpp|cpp_positive_sema_namespaced_inherited_type_alias_base_member_lookup_parse_cpp|cpp_eastl_integer_sequence_parse_recipe|cpp_eastl_type_traits_parse_recipe|cpp_eastl_utility_parse_recipe|cpp_eastl_vector_parse_recipe|cpp_eastl_memory_uses_allocator_parse_recipe|eastl_cpp_external_utility_frontend_basic_cpp)$'; } > test_after.log 2>&1`

Proof passed. `test_after.log` contains the canonical executor proof log:
12/12 selected tests passed, including `frontend_parser_tests`,
`frontend_parser_lookup_authority_tests`, the namespace using-declaration
frontend case, inherited alias-base/member lookup cases, and the delegated
EASTL regression recipes.
