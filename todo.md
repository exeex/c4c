# Current Packet

Status: Complete
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Fixture Helpers Off Direct Tag Access

## Just Finished

Step 2 migrated
`test_parser_template_instantiation_dedup_keys_structure_direct_emission()`
away from direct `first.tag`/`second.tag`/`third.tag` reads. The fixture now
uses `tag_text_id` plus `parser.parser_text()` for the instantiated spelling,
continues to assert `record_def` identity against the rendered struct map
entry, and still proves direct template emission creates, recreates, and
reuses one structured de-dup key.

The follow-up deletion probe temporarily removed `TypeSpec::tag` from
`src/frontend/parser/ast.hpp`, reran the delegated fixture-surface build probe,
captured the result in `test_after.log`, and restored `ast.hpp` afterward.
The migrated fixture no longer blocks that probe; the first remaining
fixture/test compile boundary moved to
`tests/frontend/frontend_parser_tests.cpp:6222`, where
`test_parser_template_substitution_preserves_record_definition_payloads()`
still writes `param_ts.tag`.

## Suggested Next

Migrate
`test_parser_template_substitution_preserves_record_definition_payloads()` away
from direct `TypeSpec::tag` writes/reads while preserving template payload
substitution coverage for rendered spelling compatibility and `record_def`
identity.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- The delegated `frontend_parser_tests` target build covers this fixture
  surface, but the deletion probe still shows later direct `tag` residuals.
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
`tests/frontend/frontend_parser_tests.cpp:6222`.
