# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Fixture Helpers Off Direct Tag Access

## Just Finished

Step 2 migrated the two direct legacy `alias_ts.tag` writes in
`test_parser_member_typedef_suffix_rejects_rendered_owner_fallbacks` behind
`set_legacy_tag_if_present(alias_ts, ..., 0)` while preserving the
rendered-owner fallback rejection fixture shape.

## Suggested Next

Run the next Step 3 temporary `TypeSpec::tag` deletion probe to identify the
next direct-tag fixture boundary after
`test_parser_member_typedef_suffix_rejects_rendered_owner_fallbacks`.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- Temporary deletion probes must be restored unless the packet is the final
  accepted field-removal deletion.
- The known `frontend_parser_tests` namespace-owner assertion is outside these
  fixture migration packets and should not be repaired as part of narrow stale
  tag fixture writes.

## Proof

Build proof passed and observational parser-test proof was recorded in
`test_after.log`:

```sh
cmake --build --preset default > test_after.log 2>&1
ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' >> test_after.log 2>&1
```

The CTest command returned the known pre-existing failure:
`namespace owner resolution should use the method owner TextId before rendered
owner spelling`.
