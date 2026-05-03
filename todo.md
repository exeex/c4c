# Current Packet

Status: Complete
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Fixture Helpers Off Direct Tag Access

## Just Finished

Step 2 migrated
`test_parser_alias_template_member_typedef_carrier_uses_structured_rhs()` and
the paired member-typedef substitution setup away from direct `TypeSpec::tag`
access. The carrier assertion now checks the substitutable owner arg through
`tag_text_id`, and stale rendered owner spelling is written only through
`set_legacy_tag_if_present()` while `tpl_struct_origin` keeps its independent
legacy spelling pointer.

The deletion probe temporarily removed `TypeSpec::tag` from
`src/frontend/parser/ast.hpp`, reran the delegated fixture-surface build probe,
captured the result in `test_after.log`, and restored `ast.hpp` afterward.
The migrated fixture no longer blocks that probe at the old
6865/6867/6881/6913/6914 sites.
The first remaining fixture/test compile boundary moved to
`tests/frontend/frontend_parser_tests.cpp:6963`.

## Suggested Next

Migrate the next direct `TypeSpec::tag` fixture access cluster in
`test_parser_qualified_alias_template_member_typedef_substitution_uses_structured_carrier()`
at `tests/frontend/frontend_parser_tests.cpp:6963` and `6964`, preserving the
structured member typedef carrier assertions while keeping any legacy rendered
spelling behind the compatibility helper.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- The delegated `frontend_parser_tests` target build covers this fixture
  surface, but the deletion probe still shows later direct `tag` residuals.
- The next deletion-probe boundary is another alias-template member typedef
  carrier substitution fixture; use a local stale rendered owner pointer for
  `tpl_struct_origin` instead of reading `info.aliased_type.tag`.
- The migrated tag-only fallback fixture intentionally keeps no `record_def`;
  the compatibility map remains the intended authority for the 1/2/1 layout
  results.
- The incomplete-declaration helper now depends on `record_def` as the real
  authority while keeping `Shared` rendered-map disagreement coverage through
  the stale `struct_tag_def_map` entry.
- The stale-rendered parameter disagreement coverage in the migrated fixture
  remains in `info.param_names = {"StaleRenderedT"}` versus
  `info.param_name_text_ids = {param_text}`.
- The no-rendered-param-name fixture intentionally omits `param_names`; keep
  that structured-only behavior intact.
- `src/frontend/parser/ast.hpp` was only changed for the temporary deletion
  probe and has no lasting diff.

## Proof

Proof output is recorded in `test_after.log`. The initial normal build proof
was overwritten by the temporary deletion probe, then the restored passing build
was appended to the same log.

```text
Initial normal build proof:
  cmake --build build --target frontend_parser_tests > test_after.log 2>&1

Temporary TypeSpec::tag deletion probe, overwriting the initial log:
  cmake --build build --target frontend_parser_tests > test_after.log 2>&1

Restored ast.hpp build proof, appended after the deletion probe:
  cmake --build build --target frontend_parser_tests >> test_after.log 2>&1
```

Result: the normal `frontend_parser_tests` target build passed after the
fixture migration. The deletion-probe build failed while `TypeSpec::tag` was
temporarily removed, then `ast.hpp` was restored and the target build passed
again. The first remaining compile boundary is
`tests/frontend/frontend_parser_tests.cpp:6963`.
