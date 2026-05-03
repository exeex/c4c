# Current Packet

Status: Complete
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Fixture Helpers Off Direct Tag Access

## Just Finished

Step 2 migrated
`test_parser_direct_record_types_carry_record_definition()` away from direct
`direct_ts.tag` and `union_ts.tag` reads. The fixture now proves final spelling
compatibility through `tag_text_id` plus `parser.parser_text()`, preserves the
direct struct/union `record_def` identity and record-kind assertions, and keeps
the struct compatibility tag-map entry classified as compatibility coverage by
checking the rendered string key against `struct_tag_def_map`.

The follow-up deletion probe temporarily removed `TypeSpec::tag` from
`src/frontend/parser/ast.hpp`, reran the delegated fixture-surface build probe,
captured the result in `test_after.log`, and restored `ast.hpp` afterward.
The migrated fixture no longer blocks that probe. The first remaining
fixture/test compile boundary moved to
`tests/frontend/frontend_parser_tests.cpp:6413`, where
`test_parser_tag_only_record_types_keep_null_record_definition()` still reads
`tag_only_ts.tag`.

## Suggested Next

Migrate `test_parser_tag_only_record_types_keep_null_record_definition()` away
from direct `TypeSpec::tag` reads while preserving tag-only spelling coverage
and the null `record_def` contract.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- The delegated `frontend_parser_tests` target build covers this fixture
  surface, but the deletion probe still shows later direct `tag` residuals.
- Compatibility-map checks in direct record parsing are rendered-spelling
  compatibility assertions, not semantic authority; keep `record_def` as the
  semantic identity source.
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
boundary is the tag-only fixture access at
`tests/frontend/frontend_parser_tests.cpp:6413`.
