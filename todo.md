Status: Active
Source Idea Path: ideas/open/145_move_record_tag_authority_from_parser_to_sema.md
Source Plan Path: plan.md
Current Step ID: 4C
Current Step Title: Migrate Remaining Parser Record Lookup Families

# Current Packet

## Just Finished

Step 4C C declaration-completeness migration completed. The C typedef-target
completion helper no longer scans `definition_state_.struct_tag_def_map` by
`tag_text_id`/namespace context when visible typedef metadata lacks
`record_def`; direct complete `record_def` metadata on the typedef remains
accepted. Focused C parser fixtures now seed stale complete parser-map records
and prove structured typedef misses fail for both top-level and local
declarations instead of recovering completion from parser-map TextId matches.

## Suggested Next

Supervisor should review and commit this focused Step 4C slice with
`src/frontend/parser/impl/declarations.cpp`,
`tests/frontend/frontend_parser_tests.cpp`, this `todo.md` update, and
`test_after.log` if it is tracked in the local workflow. The untracked review
artifacts remain outside this packet.

## Watchouts

The remaining `declaration_complete_object_record_def` parser-map lookup is the
legacy TextId-less compatibility branch and still rejects structured carriers
before consulting the map. This packet did not change record layout constant
evaluation or other parser-map consumers outside declaration completeness.

## Proof

Proof run:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests)$' > test_after.log`

Result: passed, 2/2 tests. Proof log: `test_after.log`.
