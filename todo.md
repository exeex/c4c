# Current Packet

Status: Active
Source Idea Path: ideas/open/123_parser_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Demote Compatibility String Helpers And Tests

## Just Finished

Step 4 packet completed: record-layout constant-evaluation helper surfaces now
label retained rendered struct-tag maps as tag-only final-spelling
compatibility fallback, while keeping `TypeSpec::record_def` as the documented
record-layout authority.

`resolve_record_type_spec()` and the record-layout `eval_const_int()` plumbing
now use `compatibility_tag_map` parameter names/comments for the rendered tag
path. The frontend parser record-layout tests were renamed and their assertions
updated to describe record-definition authority versus final-spelling fallback.
No behavior changed.

## Suggested Next

Next Step 4 packet: demote the template primary/specialization rendered-name
bridge surface without changing behavior.

Scope:
- `src/frontend/parser/impl/parser_state.hpp`: the retained
  `template_struct_defs` and `template_struct_specializations` string maps
  beside `template_struct_defs_by_key` /
  `template_struct_specializations_by_key`.
- `src/frontend/parser/impl/types/template.cpp`: registration and lookup helper
  paths around `register_template_struct_primary()`,
  `register_template_struct_specialization()`,
  `find_template_struct_primary()`, and
  `find_template_struct_specializations()`.
- `tests/frontend/frontend_parser_tests.cpp`: tests around
  `test_parser_template_lookup_demotes_legacy_rendered_name_bridges()`.

Expected change shape:
- Keep `QualifiedNameKey` tables as the documented primary lookup authority.
- Rename local variables and comments for the string maps as rendered-name
  compatibility/final-spelling mirrors or TextId-less lookup bridges.
- Keep public helper behavior intact, including TextId-less compatibility
  fallback.
- Update test names/assertions only if they still describe rendered names as
  ordinary template lookup authority.

Suggested proof for the executor packet:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' > test_after.log 2>&1`

## Watchouts

This packet intentionally did not alter `struct_tag_def_map` behavior,
`TypeSpec::record_def` behavior, or support helpers outside record-layout
constant evaluation. The rendered tag map remains functional for tag-only
TypeSpecs that lack typed record identity.

For the next packet, do not convert the template string maps or delete their
fallback reads. This is a Step 4 demotion/classification packet: behavior should
stay the same, and stale rendered-name precedence should remain rejected only
where a valid structured key/TextId path exists.

## Proof

Proof command run exactly as delegated:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' > test_after.log 2>&1`

Result: passed. `test_after.log` contains the delegated frontend parser subset
output: 1 test passed, 0 failed.
