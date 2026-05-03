# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Fixture Helpers Off Direct Tag Access

## Just Finished

Step 2 migrated the two sibling legacy rendered-owner fixture writes in
`frontend_parser_tests` off direct `legacy_owner_ts.tag` access. Both
`test_parser_qualified_functional_cast_owner_requires_structured_authority`
and
`test_parser_qualified_member_typedef_lookup_requires_structured_metadata`
now use the existing optional legacy-tag helper, preserving their
rendered-only authority rejection coverage while allowing the fixtures to
compile when `TypeSpec::tag` is absent.

## Suggested Next

Re-run the temporary `TypeSpec::tag` deletion probe to find the next
remaining direct field-access boundary after the two legacy rendered-owner
fixture writes.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- Temporary deletion probes must be restored unless the packet is the final
  accepted field-removal deletion.
- The previous deletion probe still reported later member typedef/template
  fixture reads and writes beginning around lines 2600-2751 after these two
  sibling writes; keep the next probe narrow before choosing another packet.
- The migrated record-constructor probe still preserves stale rendered spelling
  disagreement through the helper while exercising structured typedef identity
  and tagless `record_def` ownership.
- The known `frontend_parser_tests` namespace-owner assertion is outside these
  fixture migration packets and should not be repaired as part of narrow stale
  tag fixture writes.

## Proof

Packet proof is recorded in `test_after.log`.

```sh
cmake --build --preset default > test_after.log 2>&1
```

Result: passed.

```sh
ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' >> test_after.log 2>&1
```

Result: failed observationally at the known pre-existing assertion
`namespace owner resolution should use the method owner TextId before rendered
owner spelling`; no new blocker was identified for this packet.
