# Current Packet

Status: Complete
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Fixture Helpers Off Direct Tag Access

## Just Finished

Step 2 migrated
`test_parser_incomplete_decl_checks_prefer_record_definition()` away from the
direct `alias_ts.tag` write in its `make_alias_type` helper. The helper now
sets the `Shared` parser `tag_text_id`, keeps optional legacy spelling through
`set_legacy_tag_if_present()`, and preserves `record_def = real` so both the
global and local declaration subcases still accept the complete record even
when `struct_tag_def_map["Shared"]` points at stale incomplete record data.

The deletion probe temporarily removed `TypeSpec::tag` from
`src/frontend/parser/ast.hpp`, reran the delegated fixture-surface build probe,
captured the result in `test_after.log`, and restored `ast.hpp` afterward.
The migrated helper no longer blocks that probe. The first remaining
fixture/test compile boundary moved to
`tests/frontend/frontend_parser_tests.cpp:6761`.

## Suggested Next

Migrate the next direct `TypeSpec::tag` fixture access at
`tests/frontend/frontend_parser_tests.cpp:6761`, preserving the existing
fixture behavior while making it compile under the temporary field deletion.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- The delegated `frontend_parser_tests` target build covers this fixture
  surface, but the deletion probe still shows later direct `tag` residuals.
- The migrated tag-only fallback fixture intentionally keeps no `record_def`;
  the compatibility map remains the intended authority for the 1/2/1 layout
  results.
- The incomplete-declaration helper now depends on `record_def` as the real
  authority while keeping `Shared` rendered-map disagreement coverage through
  the stale `struct_tag_def_map` entry.
- `src/frontend/parser/ast.hpp` was only changed for the temporary deletion
  probe and has no lasting diff.

## Proof

Proof output is recorded in `test_after.log`.

```sh
cmake --build build --target frontend_parser_tests > test_after.log 2>&1
cmake --build build --target frontend_parser_tests >> test_after.log 2>&1
cmake --build build --target frontend_parser_tests >> test_after.log 2>&1
```

Result: the normal `frontend_parser_tests` target build passed after the
fixture migration. The deletion-probe build failed while `TypeSpec::tag` was
temporarily removed, then `ast.hpp` was restored and the target build passed
again. The first remaining compile boundary is
`tests/frontend/frontend_parser_tests.cpp:6761`.
