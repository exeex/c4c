# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Fixture Helpers Off Direct Tag Access

## Just Finished

Step 2 migrated the first `frontend_parser_tests.cpp` fixture residual at
`tests/frontend/frontend_parser_tests.cpp:4322` in
`test_parser_template_type_arg_uses_visible_scope_local_alias()` away from
direct `arg.type.tag` access. The fixture now preserves the same contract by
asserting that the visible lexical alias resolves to the scope-local `TB_INT`
type without fabricating typedef TextId or qualified typedef metadata.

The follow-up deletion probe temporarily removed `TypeSpec::tag` from
`src/frontend/parser/ast.hpp`, reran the delegated fixture-surface build probe,
captured the result in `test_after.log`, and restored `ast.hpp` afterward.
The first remaining fixture/test compile boundary moved to
`tests/frontend/frontend_parser_tests.cpp:4349`, where
`test_parser_synthesized_typedef_binding_unregisters_by_text_id()` still reads
`synthesized_type->tag`.

## Suggested Next

Migrate the next fixture residual at
`tests/frontend/frontend_parser_tests.cpp:4349` away from direct
`synthesized_type->tag` access while preserving the synthesized typedef
spelling contract through `tag_text_id` and parser-owned text lookup.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- The delegated `c4cll` proof does not compile parser fixture tests, so clean
  target build output is not final repository deletion readiness.
- The focused frontend parser test previously had the known pre-existing
  `namespace owner resolution should use the method owner TextId before rendered
  owner spelling` failure.
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
boundary is the direct fixture read at
`tests/frontend/frontend_parser_tests.cpp:4349`.
