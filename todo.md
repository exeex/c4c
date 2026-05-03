# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Re-run The TypeSpec Tag Deletion Probe

## Just Finished

Step 3 temporarily removed `TypeSpec::tag` from
`src/frontend/parser/ast.hpp` and re-ran the deletion probe. The first
remaining compile boundary is still in `frontend_parser_tests`: the earliest
error is
`test_parser_qualified_functional_cast_owner_requires_structured_authority`
at `tests/frontend/frontend_parser_tests.cpp:2283`, where
`legacy_owner_ts.tag` is written for a legacy rendered owner fixture. The next
error is the sibling legacy owner fixture at line 2373, followed by member
typedef/template fixture reads and writes beginning around lines 2600-2751.
The temporary `ast.hpp` deletion edit was restored after the probe.

## Suggested Next

Migrate the earliest remaining `frontend_parser_tests` legacy-owner fixture
boundary off direct `TypeSpec::tag` access, starting with the line 2283
`legacy_owner_ts.tag` write in
`test_parser_qualified_functional_cast_owner_requires_structured_authority`.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- Temporary deletion probes must be restored unless the packet is the final
  accepted field-removal deletion.
- The latest deletion probe still reaches `frontend_parser_tests` before
  production code; keep the next packet narrow and avoid sweeping later member
  typedef/template fixture families into the same edit.
- The line 2283 and 2373 boundaries are legacy rendered-owner fixtures that
  intentionally test rejection of rendered-only authority; preserve that
  negative coverage when migrating them.
- The migrated record-constructor probe still preserves stale rendered spelling
  disagreement through the helper while exercising structured typedef identity
  and tagless `record_def` ownership.
- The known `frontend_parser_tests` namespace-owner assertion is outside these
  fixture migration packets and should not be repaired as part of narrow stale
  tag fixture writes.

## Proof

Deletion-probe proof is recorded in `test_after.log`.

```sh
cmake --build --preset default > test_after.log 2>&1
```

Result: failed as expected with `TypeSpec::tag` temporarily removed. First
boundary:
`tests/frontend/frontend_parser_tests.cpp:2283:19: error: 'struct c4c::TypeSpec' has no member named 'tag'`.

```sh
cmake --build --preset default
```

Result after restoring `src/frontend/parser/ast.hpp`: passed.
