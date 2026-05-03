# Current Packet

Status: Complete
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Fixture Helpers Off Direct Tag Access

## Just Finished

Step 2 migrated
`test_parser_record_layout_const_eval_keeps_final_spelling_fallback()` away
from a direct `tag_only.tag` write. The fixture now assigns a `Fallback`
`tag_text_id` to the tag-only `TypeSpec`, mirrors that identity onto the
synthetic fallback record, and uses optional legacy-tag setup so builds still
compile with and without the legacy field. The fixture keeps `record_def` null
and preserves the 1/2/1 alignof/sizeof/offsetof expectations through the
compatibility map fallback.

The deletion probe temporarily removed `TypeSpec::tag` from
`src/frontend/parser/ast.hpp`, reran the delegated fixture-surface build probe,
captured the result in `test_after.log`, and restored `ast.hpp` afterward.
The migrated fixture no longer blocks that probe. The first remaining
fixture/test compile boundary moved to
`tests/frontend/frontend_parser_tests.cpp:6611`, where the adjacent
`test_parser_incomplete_decl_checks_prefer_record_definition()` helper still
writes `alias_ts.tag`.

## Suggested Next

Migrate `test_parser_incomplete_decl_checks_prefer_record_definition()` away
from direct `TypeSpec::tag` writes while preserving its stale rendered tag-map
checks against `record_def` authority.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- The delegated `frontend_parser_tests` target build covers this fixture
  surface, but the deletion probe still shows later direct `tag` residuals.
- The migrated tag-only fallback fixture intentionally keeps no `record_def`;
  the compatibility map remains the intended authority for the 1/2/1 layout
  results.
- The next boundary is an incomplete-declaration helper where `record_def`
  should remain authoritative over the rendered tag-map entry.
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
again. The first remaining compile boundary is the adjacent incomplete-decl
helper access at `tests/frontend/frontend_parser_tests.cpp:6611`.
