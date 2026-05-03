# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Re-run The TypeSpec Tag Deletion Probe

## Just Finished

Step 3 re-ran the temporary `TypeSpec::tag` deletion probe by removing the
field from `src/frontend/parser/ast.hpp` and building with the supervisor
selected proof command. The probe still fails to compile; the first remaining
boundary is `tests/frontend/frontend_parser_tests.cpp:1859`, where
`test_parser_resolve_typedef_type_chain_uses_local_visible_scope_lookup`
writes `alias_ts.tag = arena.strdup("Target")`. The temporary deletion edit was
restored afterward.

## Suggested Next

Migrate the next owned compile boundary in `tests/frontend/frontend_parser_tests.cpp`:
replace the direct `alias_ts.tag` write in
`test_parser_resolve_typedef_type_chain_uses_local_visible_scope_lookup` with
TextId-backed or helper-mediated metadata, preserving the test's local visible
scope lookup contract.

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

Result: failed as the expected deletion probe. First error:
`tests/frontend/frontend_parser_tests.cpp:1859:12: error: 'struct c4c::TypeSpec' has no member named 'tag'`.

```sh
cmake --build --preset default
```

Result: passed after restoring the temporary `src/frontend/parser/ast.hpp`
deletion edit. `test_after.log` intentionally remains the probe-build log.
