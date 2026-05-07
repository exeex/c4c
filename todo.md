Status: Active
Source Idea Path: ideas/open/146_qualified_name_deferred_carrier_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Route Eager Sema Qualified Lookup Through Domain Tables

# Current Packet

## Just Finished

Step 4: Route Eager Sema Qualified Lookup Through Domain Tables repaired the Sema `TypeSpec` equivalence regression from commit `53f343eea` by accepting exact structured partial text-name metadata matches while still blocking rendered-spelling fallback when structured metadata is stale, partial, or one-sided.

## Suggested Next

Supervisor should decide whether the two parser-debug expectation failures are an outside-packet baseline issue or delegate a separate parser-debug/test-expectation packet, then rerun acceptance.

## Watchouts

The exact delegated proof still fails `cpp_parser_debug_qualified_type_template_arg_stack` and `cpp_parser_debug_qualified_type_spelling_stack`; checking out pre-`53f343eea` `src/frontend/sema/type_utils.cpp` and running only those two tests still failed, so the remaining blocker appears outside this owned TypeSpec repair slice.

## Proof

`cd /workspaces/c4c && { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_hir_sema_consteval_type_utils_structured_metadata|cpp_positive_sema_qualified_namespaced_out_of_class_method_context_frontend_cpp|cpp_positive_sema_namespace_using_decl_typedef_and_value_frontend_cpp|cpp_parser_debug_qualified_type_template_arg_stack|cpp_parser_debug_qualified_type_spelling_stack)$'; } > test_after.log 2>&1`

Proof failed. `test_after.log` is the canonical executor proof log: `frontend_parser_tests`, `frontend_parser_lookup_authority_tests`, `cpp_hir_sema_consteval_type_utils_structured_metadata`, and both positive Sema qualified-name tests passed; the two parser-debug stack-substring tests failed with the same failure when isolated against pre-`53f343eea` `type_utils.cpp`.
