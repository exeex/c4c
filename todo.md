Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Replace remaining suitable single-name string tables and isolate holdouts

# Current Packet

## Just Finished
Completed the step 5 parser-entry-point sweep for a remaining body-parsing
holdout. Added the scoped function-parameter binding guard to the
out-of-class constructor-definition body path so constructor parameters stay
visible during body parsing, and added a frontend regression that exercises a
ctor body containing a nested ctor-init disambiguation probe.

## Suggested Next
Continue step 5 by checking whether any other parser entry points still need a
parameter-scope bridge for lexical disambiguation. If no additional holdouts
show up, keep the next slice focused on the remaining parser/frontend cleanup
items under the active idea.

## Watchouts
Keep the new parameter scope limited to body parsing so declaration-only paths
do not start inheriting function parameters unexpectedly. Qualified concept
lookup still needs to stay on structured namespace-context keys.

## Proof
Ran `cmake --build --preset default` and
`ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_parse_record_member_template_friend_cleanup_dump|cpp_positive_sema_record_template_prelude_and_tag_registration_parse_cpp|cpp_positive_sema_local_direct_init_paren_runtime_cpp|cpp_positive_sema_local_direct_init_single_identifier_runtime_cpp)$'`
with proof captured in `test_after.log`.
