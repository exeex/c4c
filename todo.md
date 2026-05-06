Status: Active
Source Idea Path: ideas/open/145_move_record_tag_authority_from_parser_to_sema.md
Source Plan Path: plan.md
Current Step ID: 4C
Current Step Title: Migrate Remaining Parser Record Lookup Families

# Current Packet

## Just Finished

Step 4C parser record-constructor classification migration completed. The
structured helper no longer scans
`definition_state_.struct_tag_def_map` by `tag_text_id`/namespace context, and
the parser core record-projection classifier no longer consults
`resolve_record_type_spec` or `definition_state_.struct_tag_def_map` when
`record_def` metadata is missing. The focused lookup-authority fixture now
proves a stale rendered `struct_tag_def_map` entry cannot make projection
classification succeed, while direct `record_def` metadata remains accepted.

## Suggested Next

Supervisor should review and commit this focused Step 4C slice with
`src/frontend/parser/impl/core.cpp`,
`tests/frontend/frontend_parser_lookup_authority_tests.cpp`, this `todo.md`
update, and `test_after.log` if it is tracked in the local workflow. The
untracked `review/step4c_repair_route_review.md` remains outside this packet.

## Watchouts

Structured-metadata misses and no-metadata typedef probes still fail as
expected. This packet did not add parser-map lookup, rendered-name recovery,
debug-text recovery, `@origin` string recovery, or unique TextId parser-map
recovery. The remaining `struct_tag_def_map` reference in `core.cpp` belongs to
the separate legacy record-constructor compatibility fallback, not projection
classification.

## Proof

Proof run:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_hir_parser_core_record_ctor_structured_metadata)$' > test_after.log`

Result: passed, 3/3 tests. Proof log: `test_after.log`.
