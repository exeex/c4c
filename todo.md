Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Reduce legacy lexical bridge lookup to compatibility-only support

# Current Packet

## Just Finished
Closed out step 5 after confirming the remaining parser single-name lexical
tables in the active surface are already `TextId`-first. The leftover
`known_fn_names` and `struct_typedefs` paths are structured-qualified holdouts
that the idea explicitly keeps out of this slice.

## Suggested Next
Start step 6 by narrowing the touched lexical visible-name helpers so bridge
string lookup stays compatibility-only after scope-local `TextId` matches and
structured namespace probes have already had their chance.

## Watchouts
Do not pull `known_fn_names` or `struct_typedefs` back into flat string-key
work. The next packet should target lexical fallback demotion on the already
touched unqualified type/value/concept paths only.

## Proof
No new command for this lifecycle-only state update. The last code packet proof
remains the `cmake --build --preset default` plus
`ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_positive_sema_record_member_template_scope_cleanup_parse_cpp|cpp_positive_sema_template_declaration_prelude_cleanup_parse_cpp|cpp_parse_record_member_template_friend_cleanup_dump)$'`
run recorded in `test_after.log`.
