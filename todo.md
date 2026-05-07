Status: Active
Source Idea Path: ideas/open/146_qualified_name_deferred_carrier_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Route Eager Sema Qualified Lookup Through Domain Tables

# Current Packet

## Just Finished

Step 4: Route Eager Sema Qualified Lookup Through Domain Tables resolved the two parser-debug baseline failures by aligning the diagnostic-format expectations to the preserved qualified-spelling wrapper frames now emitted before the nested qualified probe, keeping the committed parse failure and stack depth checks intact.

## Suggested Next

Supervisor can review and commit this expectation-alignment slice with the green delegated parser-debug/parser-authority proof.

## Watchouts

This is diagnostic-format alignment, not a parser weakening: the actual stacks retain the same `parse_top_level_parameter_list` committed failure leaf while preserving additional leading `consume_qualified_type_spelling_with_typename` / `consume_qualified_type_spelling` context from the eager qualified-spelling probe.

## Proof

`cd /workspaces/c4c && { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_parser_debug_qualified_type_template_arg_stack|cpp_parser_debug_qualified_type_spelling_stack|cpp_parser_debug_tentative_template_arg_lifecycle|frontend_parser_tests|frontend_parser_lookup_authority_tests)$'; } > test_after.log 2>&1`

Proof passed. `test_after.log` is the canonical executor proof log: both parser-debug qualified stack tests, the tentative template-arg lifecycle test, `frontend_parser_tests`, and `frontend_parser_lookup_authority_tests` passed.
