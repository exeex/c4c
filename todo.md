# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Fixture Helpers Off Direct Tag Access

## Just Finished

Step 2 migrated the `make_ts` lambda inside
`test_sema_namespace_owner_resolution_prefers_structured_owner_key()` away from
the direct `ts.tag = tag` write. The helper now carries structured
`tag_text_id`, `record_def`, and namespace metadata for the real `Owner` return
type, while optional legacy rendered compatibility setup goes through
`set_legacy_tag_if_present()`.

The follow-up deletion probe temporarily removed `TypeSpec::tag` from
`src/frontend/parser/ast.hpp`, reran the delegated fixture-surface build probe,
captured the result in `test_after.log`, and restored `ast.hpp` afterward.
The migrated fixture no longer blocks that probe; the first remaining
fixture/test compile boundary moved to
`tests/frontend/frontend_parser_tests.cpp:5744`, where the next sema namespace
owner `make_ts` lambda still writes `ts.tag = tag`.

## Suggested Next

Migrate the next fixture residual at
`tests/frontend/frontend_parser_tests.cpp:5744` away from direct `ts.tag`
access while preserving the rendered-fallback rejection contract in
`test_sema_namespace_owner_resolution_rejects_rendered_fallback_after_structured_miss()`.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- The delegated `frontend_parser_tests` target build covers this fixture
  surface, but the deletion probe still shows later direct `tag` residuals.
- `src/frontend/parser/ast.hpp` was only changed for the temporary deletion
  probe and should have no lasting diff.

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
`tests/frontend/frontend_parser_tests.cpp:5744`.
