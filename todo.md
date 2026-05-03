# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Re-run The TypeSpec Tag Deletion Probe

## Just Finished

Step 3 re-ran the temporary `TypeSpec::tag` deletion probe by removing the
field from `src/frontend/parser/ast.hpp` and running the delegated build. The
probe still fails at the test-fixture boundary, with the first remaining
compile error in
`tests/frontend/frontend_parser_tests.cpp:1611` inside
`test_parser_record_ctor_probe_prefers_record_definition`:
`alias_ts.tag = arena.strdup("StaleBox");`.

The temporary `ast.hpp` deletion edit was restored after the probe.

## Suggested Next

Migrate the direct legacy `TypeSpec::tag` fixture writes in
`test_parser_record_ctor_probe_prefers_record_definition` behind a helper or
structured metadata path, preserving the stale-rendered-spelling coverage
without weakening the test contract.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- Temporary deletion probes must be restored unless the packet is the final
  accepted field-removal deletion.
- The next compile boundary is in `tests/frontend/frontend_parser_tests.cpp`,
  which was read-only for this packet.
- The known `frontend_parser_tests` namespace-owner assertion is outside these
  fixture migration packets and should not be repaired as part of narrow stale
  tag fixture writes.

## Proof

Proof is recorded in `test_after.log`.

```sh
cmake --build --preset default > test_after.log 2>&1
```

Result for the temporary deletion probe: failed as expected. The first
remaining compile boundary is
`tests/frontend/frontend_parser_tests.cpp:1611`, direct fixture access to
`alias_ts.tag` in `test_parser_record_ctor_probe_prefers_record_definition`.

```sh
cmake --build --preset default
```

Result after restoring `src/frontend/parser/ast.hpp`: passed.
