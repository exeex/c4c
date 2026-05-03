# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Fixture Helpers Off Direct Tag Access

## Just Finished

Step 2 migrated
`test_parser_deferred_member_typedef_lookup_uses_member_text_id` off direct
`TypeSpec::tag` setup for the local `box_alias` and `arg_ts` bindings.
`box_alias` now uses `tag_text_id` plus `record_def`, `arg_ts` now uses
`tag_text_id`, `record_def`, and `deferred_member_type_text_id`, and the
synthetic owner/template nodes carry the local member typedef and template
parameter `TextId` metadata needed by the structured assertion.

## Suggested Next

Continue Step 2 with the next direct `TypeSpec::tag` fixture boundary reported
by the deletion probe, without sweeping unrelated later uses in the file.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- Temporary deletion probes must be restored unless the packet is the final
  accepted field-removal deletion; `src/frontend/parser/ast.hpp` was restored
  after this probe.
- The next visible direct `TypeSpec::tag` use in
  `tests/frontend/frontend_parser_tests.cpp` is in the following template-base
  deferred member typedef fixture; handle it as a separate coherent packet.

## Proof

Build plus focused parser-test output is recorded in `test_after.log`.

```sh
cmake --build --preset default > test_after.log 2>&1; ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' >> test_after.log 2>&1
```

Result: command exited nonzero with the known focused-test failure only:
`namespace owner resolution should use the method owner TextId before rendered
owner spelling`. This matches the failure set in `test_before.log`.
