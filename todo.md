Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Replace remaining suitable single-name string tables and isolate holdouts

# Current Packet

## Just Finished
Completed the step 5 template-scope TextId-first cleanup slice. Updated the
template-scope parameter probe so semantic `TextId` matches win whenever active
template params carry them, while spelling-only holdouts still work when no
semantic ids are present. Added a focused frontend regression for the mixed
TextId/spelling case.

## Suggested Next
Continue step 5 by checking the remaining parser single-name lookup holdouts in
the owned parser surface and isolate any places that still need spelling-based
fallbacks.

## Watchouts
Template-scope lookup now prefers semantic ids whenever they are available, but
legacy spelling-only frames still rely on the fallback path. Keep that fallback
isolated so it does not become the primary route again.

## Proof
Ran `cmake --build --preset default` and
`ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_positive_sema_record_member_template_scope_cleanup_parse_cpp|cpp_positive_sema_template_declaration_prelude_cleanup_parse_cpp|cpp_parse_record_member_template_friend_cleanup_dump)$'`
with proof captured in `test_after.log`.
