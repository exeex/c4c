# Current Packet

Status: Complete
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Fixture Helpers Off Direct Tag Access

## Just Finished

Step 2 migrated
`test_parser_template_substitution_preserves_record_definition_payloads()` away
from direct `param_ts.tag`, `box_alias.tag`, `payload_alias.tag`, `box_ts.tag`,
and `box->base_types[0].tag` access. The fixture now seeds template parameter,
Box, and Payload identities through `tag_text_id` plus parser text ids, wires
`record_def` on the structured aliases, and keeps the rendered spelling
compatibility assertion by checking the substituted base `tag_text_id` through
`parser.parser_text()`.

The follow-up deletion probe temporarily removed `TypeSpec::tag` from
`src/frontend/parser/ast.hpp`, reran the delegated fixture-surface build probe,
captured the result in `test_after.log`, and restored `ast.hpp` afterward.
The migrated fixture no longer blocks that probe. The first remaining
fixture/test compile boundary moved to
`tests/frontend/frontend_parser_tests.cpp:6362`, where
`test_parser_direct_record_types_carry_record_definition()` still reads
`direct_ts.tag`.

## Suggested Next

Migrate `test_parser_direct_record_types_carry_record_definition()` away from
direct `TypeSpec::tag` reads while preserving direct record parsing coverage
for rendered spelling compatibility and `record_def` identity.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- The delegated `frontend_parser_tests` target build covers this fixture
  surface, but the deletion probe still shows later direct `tag` residuals.
- `test_parser_template_substitution_preserves_record_definition_payloads()`
  now relies on structured template parameter metadata; keep that coverage
  intact if nearby template fixture helpers are adjusted later.
- `src/frontend/parser/ast.hpp` was only changed for the temporary deletion
  probe and has no lasting diff.

## Proof

Proof output is recorded in `test_after.log`.

```sh
cmake --build build --target frontend_parser_tests > test_after.log 2>&1
cmake --build build --target frontend_parser_tests >> test_after.log 2>&1
```

Result: the normal `frontend_parser_tests` target build passed after the
fixture migration. The deletion-probe build failed while `TypeSpec::tag` was
temporarily removed, then `ast.hpp` was restored. The first remaining compile
boundary is the direct fixture access at
`tests/frontend/frontend_parser_tests.cpp:6362`.
