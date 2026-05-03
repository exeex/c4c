# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Fixture Helpers Off Direct Tag Access

## Just Finished

Step 2 migrated the direct `alias_ts.tag` fixture write in
`test_parser_resolve_typedef_type_chain_uses_local_visible_scope_lookup` to
`alias_ts.tag_text_id`, reusing the `Target` `TextId` that is bound in the
local visible typedef scope. The local visible scope lookup assertion remains
unchanged.

## Suggested Next

Re-run the temporary `TypeSpec::tag` deletion probe to identify the next direct
tag fixture write after the migrated local visible scope typedef-chain case.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- Temporary deletion probes must be restored unless the packet is the final
  accepted field-removal deletion.
- The latest deletion probe surfaced many remaining test-only `TypeSpec::tag`
  accesses after the first boundary; keep the next packet narrow and avoid
  sweeping unrelated parser/HIR fixture families into one edit.
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
owner spelling` assertion; kept as observational parser-test output for this
packet.
