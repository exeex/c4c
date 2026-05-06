Status: Active
Source Idea Path: ideas/open/145_move_record_tag_authority_from_parser_to_sema.md
Source Plan Path: plan.md
Current Step ID: 4C
Current Step Title: Migrate Remaining Parser Record Lookup Families

# Current Packet

## Just Finished

Step 4C remaining parser record lookup-family migration completed.
`record_definition_in_context_by_text_id` now resolves context/TextId record
matches from parser structured `struct_defs` instead of scanning
`definition_state_.struct_tag_def_map`. Its existing caller families now inherit
that authority boundary, and focused parser coverage verifies stale/conflicting
tag-map candidates cannot recover record identity while direct structured
record definitions still resolve and ambiguity remains rejected.

## Suggested Next

Supervisor should review and commit this Step 4C record lookup helper slice
with `src/frontend/parser/impl/types/types_helpers.hpp`,
`tests/frontend/frontend_parser_tests.cpp`, this `todo.md` update, and
`test_after.log` if it is tracked in the local workflow. The untracked review
artifacts remain outside this packet.

## Watchouts

Several older parser tests used `register_struct_definition_for_testing` as a
rendered tag-map helper even for positive structured-authority fixtures. This
packet updated only the affected positive fixtures to seed `struct_defs`; the
legacy/rendered-only negative fixtures remain map-only. Remaining
`struct_tag_def_map` references outside `record_definition_in_context_by_text_id`
are outside this packet.

## Proof

Proof run:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests)$' > test_after.log`

Result: passed, 2/2 tests. Proof log: `test_after.log`.
