# Current Packet

Status: Complete
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Fixture Helpers Off Direct Tag Access

## Just Finished

Step 2 migrated the `make_ts` lambda inside
`test_sema_method_validation_rejects_stale_rendered_field_spelling()` away
from the direct `ts.tag = tag` write. The helper now routes optional legacy
rendered compatibility setup through `set_legacy_tag_if_present()`, while
preserving the stale rendered field spelling rejection contract and the
existing negative method-validation assertion.

The follow-up deletion probe temporarily removed `TypeSpec::tag` from
`src/frontend/parser/ast.hpp`, reran the delegated fixture-surface build probe,
captured the result in `test_after.log`, and restored `ast.hpp` afterward.
The migrated fixture no longer blocks that probe; the first remaining
fixture/test compile boundary moved to
`tests/frontend/frontend_parser_tests.cpp:6157`, where
`test_parser_template_instantiation_dedup_keys_structure_direct_emission()`
still reads `first.tag` after parsing a template struct instantiation.

## Suggested Next

Migrate the next fixture residual at
`tests/frontend/frontend_parser_tests.cpp:6157` away from direct `TypeSpec::tag`
access while preserving the direct template-instantiation emission contract.

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
`tests/frontend/frontend_parser_tests.cpp:6157`.
