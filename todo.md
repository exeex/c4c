# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Fixture Helpers Off Direct Tag Access

## Just Finished

Step 2 migrated the direct legacy `TypeSpec::tag` writes in
`test_parser_record_ctor_probe_prefers_record_definition`. The stale rendered
alias spelling now goes through `set_legacy_tag_if_present`, the typedef probe
uses `tag_text_id`, and the tagless record-def probe is built as an explicit
structured `TypeSpec` without copying and clearing a legacy tag field.

## Suggested Next

Re-run the temporary `TypeSpec::tag` deletion probe to find the next direct
fixture access that blocks removing the field.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- Temporary deletion probes must be restored unless the packet is the final
  accepted field-removal deletion.
- The migrated record-constructor probe still preserves stale rendered spelling
  disagreement through the helper while exercising structured typedef identity
  and tagless `record_def` ownership.
- The known `frontend_parser_tests` namespace-owner assertion is outside these
  fixture migration packets and should not be repaired as part of narrow stale
  tag fixture writes.

## Proof

Proof is recorded in `test_after.log`.

```sh
cmake --build --preset default > test_after.log 2>&1
```

Result: passed.

```sh
ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' >> test_after.log 2>&1
```

Result: failed only at the known pre-existing
`namespace owner resolution should use the method owner TextId before rendered
owner spelling` assertion; no new frontend parser test failure was observed for
this packet.
