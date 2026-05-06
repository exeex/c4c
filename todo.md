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
`record_def` metadata is missing. The remaining record-constructor
classification fallback no longer treats `definition_state_.defined_struct_tags`
or `definition_state_.struct_tag_def_map` as semantic record identity for
rendered/tag-only typedef probes; the retained branch is documented as
template-primary compatibility only. Focused metadata fixtures now seed stale
`defined_struct_tags` and `struct_tag_def_map` entries and still prove
structured metadata succeeds while structured misses and no-metadata typedefs
fail.

## Suggested Next

Supervisor should review and commit this focused Step 4C slice with
`src/frontend/parser/impl/core.cpp`,
`tests/frontend/cpp_hir_parser_core_record_ctor_metadata_test.cpp`,
`tests/frontend/frontend_parser_lookup_authority_tests.cpp`, this `todo.md`
update, and `test_after.log` if it is tracked in the local workflow. The
untracked review artifacts remain outside this packet.

## Watchouts

Structured-metadata misses and no-metadata typedef probes still fail as
expected. This packet did not add parser-map lookup, rendered-name recovery,
debug-text recovery, `@origin` string recovery, or unique TextId parser-map
recovery. Any remaining `struct_tag_def_map` references in parser core belong to
other lookup families, not record-constructor classification identity.

## Proof

Proof run:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_hir_parser_core_record_ctor_structured_metadata)$' > test_after.log`

Result: passed, 3/3 tests. Proof log: `test_after.log`.
