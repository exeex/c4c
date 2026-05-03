# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Fixture Helpers Off Direct Tag Access

## Just Finished

Step 2 migrated
`test_parser_template_base_deferred_member_typedef_uses_member_text_id` off its
local direct `TypeSpec::tag` fixture setup. The `Owner::Alias` base carrier now
uses `tag_text_id`, `record_def`, `member_typedef_text_ids`, and
`deferred_member_type_text_id`; the `Box` alias now uses `tag_text_id` plus
`record_def`, with template parameter TextId metadata on the primary template.

## Suggested Next

Re-run the temporary `TypeSpec::tag` deletion probe to identify the next
compile boundary after this fixture migration.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- The focused frontend parser test still has the known pre-existing
  `namespace owner resolution should use the method owner TextId before rendered
  owner spelling` failure.
- A later direct `box_alias.tag` setup remains in
  `tests/frontend/frontend_parser_tests.cpp` outside this packet's local
  fixture boundary.

## Proof

Proof output is recorded in `test_after.log`.

```sh
cmake --build --preset default > test_after.log 2>&1; ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' >> test_after.log 2>&1
```

Result: command exited nonzero. The build completed and linked
`frontend_parser_tests`; the focused test failed only with the known
pre-existing assertion:
`FAIL: namespace owner resolution should use the method owner TextId before
rendered owner spelling`. The failure set stayed the same as the delegated
baseline context.
