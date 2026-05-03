# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Fixture Helpers Off Direct Tag Access

## Just Finished

Step 2 migrated the deferred NTTP visible template-alias fixture residual in
`test_parser_deferred_nttp_member_lookup_uses_visible_scope_local_aliases()`
away from direct `trait_alias_ts.tag` access. The local `AliasTrait` binding
now preserves the visible alias lookup contract through the parser-owned
`Trait` `tag_text_id` and the registered template primary `record_def`.

The follow-up deletion probe temporarily removed `TypeSpec::tag` from
`src/frontend/parser/ast.hpp`, reran the delegated fixture-surface build probe,
captured the result in `test_after.log`, and restored `ast.hpp` afterward.
The first remaining fixture/test compile boundary moved to
`tests/frontend/frontend_parser_tests.cpp:4820`, where
`test_parser_template_static_member_lookup_prefers_record_definition()` still
writes `specialization->base_types[0].tag`.

## Suggested Next

Migrate the next fixture residual at
`tests/frontend/frontend_parser_tests.cpp:4820` away from direct
`specialization->base_types[0].tag` access while preserving the template static
member lookup contract through structured base-type metadata.

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
`tests/frontend/frontend_parser_tests.cpp:4820`.
