Status: Active
Source Idea Path: ideas/open/146_qualified_name_deferred_carrier_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Remove Parser Reliance On Rendered Qualified-Name Compatibility

# Current Packet

## Just Finished

Step 3: Remove Parser Reliance On Rendered Qualified-Name Compatibility
bounded parser core rendered-qualified spelling compatibility as
compatibility/display support rather than primary semantic authority. Concept
registration now rejects rendered compound `TextId` inputs and continues to
require an unqualified `TextId` plus context or an explicit structured key.

Focused parser coverage now checks that rendered qualified `TextId` inputs do
not re-enter structured concept lookup authority.

## Suggested Next

Review and commit this Step 3 parser-core compatibility demotion slice, then
choose the next bounded qualified-name authority packet.

## Watchouts

Qualified value/type/function authority should continue to enter through
structured `QualifiedNameKey` or context-plus-unqualified-`TextId` APIs. The
remaining rendered-qualified compatibility helper path is still needed by
qualified template parser support and should stay bounded away from primary
semantic registration paths.

## Proof

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|frontend_parser_tests|cpp_positive_sema_namespace_using_decl_typedef_and_value_frontend_cpp|cpp_positive_sema_qualified_namespaced_out_of_class_method_context_frontend_cpp)$'; } > test_after.log 2>&1`

Proof passed. `test_after.log` contains the canonical executor proof log:
4/4 selected tests passed, including `frontend_parser_lookup_authority_tests`,
`frontend_parser_tests`, the namespace using-declaration frontend case, and the
qualified namespaced out-of-class method context frontend case.
