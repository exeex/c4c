# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Fixture Helpers Off Direct Tag Access

## Just Finished

Step 2 migrated the synthesized typedef fixture residual at
`tests/frontend/frontend_parser_tests.cpp:4349` in
`test_parser_synthesized_typedef_binding_unregisters_by_text_id()` away from
direct `synthesized_type->tag` access. The fixture now preserves the same
contract by asserting that the synthesized typedef TypeSpec carries the
semantic `tag_text_id` and that `parser.parser_text(tag_text_id)` resolves to
`SynthParam`.

The follow-up deletion probe temporarily removed `TypeSpec::tag` from
`src/frontend/parser/ast.hpp`, reran the delegated fixture-surface build probe,
captured the result in `test_after.log`, and restored `ast.hpp` afterward.
The first remaining fixture/test compile boundary moved to
`tests/frontend/frontend_parser_tests.cpp:4462`, where
`test_parser_deferred_nttp_member_lookup_uses_visible_scope_local_aliases()`
still writes `trait_alias_ts.tag`.

## Suggested Next

Migrate the next fixture residual at
`tests/frontend/frontend_parser_tests.cpp:4462` away from direct
`trait_alias_ts.tag` access while preserving the visible local alias lookup
contract through structured TypeSpec metadata or parser-owned TextId spelling.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- The delegated `frontend_parser_tests` target build covers this fixture
  surface, but the deletion probe still shows later direct `tag` residuals.
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
boundary is the direct fixture access at
`tests/frontend/frontend_parser_tests.cpp:4462`.
