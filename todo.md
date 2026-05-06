Status: Active
Source Idea Path: ideas/open/145_move_record_tag_authority_from_parser_to_sema.md
Source Plan Path: plan.md
Current Step ID: 4C
Current Step Title: Migrate Remaining Parser Record Lookup Families

# Current Packet

## Just Finished

Step 4C remaining parser type-attribute constant-layout migration completed.
`parse_attributes` now routes `aligned(...)` and `vector_size(...)` expression
evaluation through `Parser::eval_const_int_with_parser_tables` instead of
directly passing `definition_state_.struct_tag_def_map`. Focused parser
coverage now drives both attribute families with `sizeof(Alias)` while a stale
parser tag-map candidate is present, and verifies the direct complete
`record_def` layout wins.

## Suggested Next

Supervisor should review and commit this Step 4C type-attribute cleanup slice
with `src/frontend/parser/impl/types/base.cpp`,
`tests/frontend/frontend_parser_tests.cpp`, this `todo.md` update, and
`test_after.log` if it is tracked in the local workflow. The untracked review
artifacts remain outside this packet.

## Watchouts

`Parser::eval_const_int_with_parser_tables` still passes the parser tag map to
the shared evaluator by design, but
`resolve_record_type_spec_for_constant_layout` limits map recovery to TextId-less
legacy carriers. The remaining `struct_tag_def_map` references in
`src/frontend/parser/impl/types/base.cpp` are outside this packet's
type-attribute constant-layout call sites.

## Proof

Proof run:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests)$' > test_after.log`

Result: passed, 2/2 tests. Proof log: `test_after.log`.
