Status: Active
Source Idea Path: ideas/open/146_qualified_name_deferred_carrier_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Separate Unqualified `TextId` APIs From Structured Qualified APIs

# Current Packet

## Just Finished

Step 2: Separate Unqualified `TextId` APIs From Structured Qualified APIs
completed the parser value/function migration slice. The `TextId`
value/function wrapper boundaries in `core.cpp` reject rendered compound
spellings for value lookup/registration and known-function key helpers, while
structured `QualifiedNameKey` paths remain the authority for qualified names.

The namespace global-variable declaration path in
`src/frontend/parser/impl/declarations.cpp` now registers from the original
unqualified declaration `TextId` while the namespace context is active, and no
longer falls back to rendered compound `gname` spellings such as
`api::payload` or `lib::seed`.

## Suggested Next

Continue Step 2 by checking for any remaining parser-facing compound `TextId`
value/function call sites outside the completed global-variable declaration
path, with special attention to using-value alias compatibility code before
moving to the next parser binding family.

## Watchouts

Do not reintroduce rendered compound splitting in
`register_var_type_binding()` or known-function `TextId` helpers. Qualified
value/function authority should continue to enter through structured
`QualifiedNameKey` or context-plus-unqualified-`TextId` APIs.

## Proof

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|frontend_parser_tests|cpp_positive_sema_namespace_using_decl_typedef_and_value_frontend_cpp|cpp_positive_sema_qualified_namespaced_out_of_class_method_context_frontend_cpp)$'; } > test_after.log 2>&1`

Proof passed. `test_after.log` contains the canonical executor proof log.
Passing: `frontend_parser_lookup_authority_tests`, `frontend_parser_tests`,
`cpp_positive_sema_namespace_using_decl_typedef_and_value_frontend_cpp`, and
`cpp_positive_sema_qualified_namespaced_out_of_class_method_context_frontend_cpp`.
