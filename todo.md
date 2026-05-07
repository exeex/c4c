Status: Active
Source Idea Path: ideas/open/146_qualified_name_deferred_carrier_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Separate Unqualified `TextId` APIs From Structured Qualified APIs

# Current Packet

## Just Finished

Step 2: Separate Unqualified `TextId` APIs From Structured Qualified APIs
completed the parser `using` typedef import registration repair.
Namespace-scope `using ns::Alias;` typedef imports now register the imported
name through `register_structured_typedef_binding_in_context()` using the
importing namespace context plus the unqualified `TextId`. The old rendered
`wrap::Alias` mirror registration was removed from this path after the
structured binding had already proven sufficient authority.

Focused parser coverage now checks that namespace typedef using imports are
available through structured/context-plus-unqualified lookup and do not create a
rendered compound `TextId` mirror.

## Suggested Next

Review and commit this Step 2 typedef using-import authority slice, then choose
the next bounded qualified-name authority packet.

## Watchouts

Do not reintroduce rendered compound splitting in
`register_var_type_binding()` or known-function `TextId` helpers. Qualified
value/function authority should continue to enter through structured
`QualifiedNameKey` or context-plus-unqualified-`TextId` APIs.

The using-alias form `using Alias = ...` still has a separate rendered typedef
compatibility mirror in the nearby alias-definition branch; this packet only
owned the using-declaration typedef import branch around
`resolve_qualified_type(target_name)`.

## Proof

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|frontend_parser_tests|cpp_positive_sema_namespace_using_decl_typedef_and_value_frontend_cpp|cpp_positive_sema_qualified_namespaced_out_of_class_method_context_frontend_cpp|cpp_positive_sema_inherited_type_alias_base_member_lookup_parse_cpp|cpp_positive_sema_namespaced_inherited_type_alias_base_member_lookup_parse_cpp|cpp_eastl_integer_sequence_parse_recipe|cpp_eastl_type_traits_parse_recipe|cpp_eastl_utility_parse_recipe|cpp_eastl_vector_parse_recipe|cpp_eastl_memory_uses_allocator_parse_recipe|eastl_cpp_external_utility_frontend_basic_cpp)$'; } > test_after.log 2>&1`

Proof passed. `test_after.log` contains the canonical executor proof log:
12/12 selected tests passed, including `frontend_parser_tests`,
`frontend_parser_lookup_authority_tests`, the namespace using-declaration
frontend case, inherited alias-base/member lookup cases, and the delegated
EASTL regression recipes.
