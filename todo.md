# Current Packet

Status: Complete
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Fixture Helpers Off Direct Tag Access

## Just Finished

Step 2 migrated
`test_parser_record_layout_const_eval_uses_record_definition_authority()` away
from a direct `typed.tag` write. The fixture now assigns `tag_text_id` for the
`Shared` spelling and uses optional legacy-tag setup while preserving
`record_def = real` against `compatibility_tag_map["Shared"] = stale`, so the
alignof/sizeof/offsetof expectations still prove record-definition authority
before stale rendered-spelling fallback.

The deletion probe temporarily removed `TypeSpec::tag` from
`src/frontend/parser/ast.hpp`, reran the delegated fixture-surface build probe,
captured the result in `test_after.log`, and restored `ast.hpp` afterward.
The migrated fixture no longer blocks that probe. The first remaining
fixture/test compile boundary moved to
`tests/frontend/frontend_parser_tests.cpp:6573`, where the adjacent
`test_parser_record_layout_const_eval_keeps_final_spelling_fallback()` fixture
still writes `tag_only.tag`.

## Suggested Next

Migrate `test_parser_record_layout_const_eval_keeps_final_spelling_fallback()`
away from direct `TypeSpec::tag` writes while preserving tag-only final-spelling
fallback coverage.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- The delegated `frontend_parser_tests` target build covers this fixture
  surface, but the deletion probe still shows later direct `tag` residuals.
- The next boundary is the tag-only fallback fixture, where the compatibility
  map is the intended authority because there is no `record_def`; migrate the
  fixture setup without weakening that fallback contract.
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
again. The first remaining compile boundary is the adjacent tag-only fallback
fixture access at `tests/frontend/frontend_parser_tests.cpp:6573`.
