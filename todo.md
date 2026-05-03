# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Fixture Helpers Off Direct Tag Access

## Just Finished

Step 2 migrated the sema cast-expression fixture helper
`make_sema_cast_expr_function()` away from direct `cast->type.tag` writes.
The helper now routes the optional stale rendered cast target spelling through
`set_legacy_tag_if_present()` so the legacy-rendered sema route is preserved
when the compatibility field exists, while the helper and both stale-name
callers compile when `TypeSpec::tag` is temporarily removed.

The follow-up deletion probe temporarily removed `TypeSpec::tag` from
`src/frontend/parser/ast.hpp`, reran the delegated fixture-surface build probe,
captured the result in `test_after.log`, and restored `ast.hpp` afterward.
The migrated fixture no longer blocks that probe; the first remaining
fixture/test compile boundary moved to
`tests/frontend/frontend_parser_tests.cpp:5662`, where a lambda still writes
`ts.tag = tag`.

## Suggested Next

Migrate the next fixture residual at
`tests/frontend/frontend_parser_tests.cpp:5662` away from direct `ts.tag`
access while preserving the surrounding sema lookup/call fixture contract.

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
`tests/frontend/frontend_parser_tests.cpp:5662`.
