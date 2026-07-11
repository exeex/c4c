Status: Active
Source Idea Path: ideas/open/701_route_fact_test_dump_contract_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove the cleanup did not weaken coverage

# Current Packet

## Just Finished

Step 2 rewrote the selected Route 4 block-entry publication
attribution/printer vocabulary behind the named prepared block-entry
publication proof surface.

Files changed:

- `src/backend/prealloc/value_locations.hpp`: added named
  `block_entry_publication_proof_*` query/result fields while retaining legacy
  `route4_*` compatibility mirrors.
- `src/backend/prealloc/prepared_lookups.cpp`: renamed the attribution helper
  to `attribute_block_entry_publication_proof_if_agreeing()`, made it prefer
  the named proof inputs, and mirrored results back to legacy compatibility
  fields when old callers still use them.
- `src/backend/prealloc/prepared_printer/value_locations.cpp`: renamed the
  prepared-printer agreement helper to
  `find_agreeing_block_entry_publication_proof()`.
- `tests/backend/bir/backend_prealloc_block_entry_publications_test.cpp`:
  moved the selected assertions to named block-entry publication proof wording
  and fields.

Remaining compatibility labels:

- Legacy `route4_successor_block`, `route4_destination_value`, and
  `route4_block_entry_publication_*` fields remain as compatibility mirrors
  because an unselected prepared-printer test surface still consumes them.
- The internal call to `bir::route4_block_entry_publication_record()` remains
  private compatibility implementation for validating BIR block-entry
  publication proof agreement.

No executable prepared availability or object behavior was changed; the named
prepared block-entry publication contract remains the proof owner.

## Suggested Next

Delegate Step 3 to decide whether the focused proof recorded below is
sufficient for the runbook proof step, or run any supervisor-selected broader
validation if needed.

## Watchouts

- Ideas 647 and 655 remain parked until positive prepared/prealloc producer
  evidence plus matching negative fail-closed proof exists for one named
  residual stack-destination producer family.
- Do not use route dump spelling as proof for executable behavior when
  prepared, MIR, object, object-runtime, or runtime proof is available.
- Do not weaken expectations, unsupported markers, allowlists, timeout policy,
  or default harness contracts.
- Do not retain the same route-only assertion under a renamed expected-output
  file and claim cleanup progress.
- Do not select Route 5 in the next packet; the handoff notes defer Route 5
  because it mixes real CFG-edge publication facts with `route5_*` proof
  residue.
- Do not broaden into Route 7 comparison vocabulary in this packet; it has its
  own AArch64 comparison proof surface and should be a separate family.
- The remaining `route4_*` fields are compatibility mirrors only. Do not treat
  them as the public/default proof vocabulary for new assertions.

## Proof

Fresh proof passed and is recorded in `test_after.log`.

Command run:

```sh
cmake --preset default -DC4C_ENABLE_PREPARED_FACT_TESTS=ON && cmake --build --preset default --target backend_prealloc_block_entry_publications_test c4cll && ctest --test-dir build -R '^(backend_prealloc_block_entry_publications|backend_cli_dump_prepared_bir_exposes_contract_sections)$' --output-on-failure | tee test_after.log
```

Result: `backend_prealloc_block_entry_publications` and
`backend_cli_dump_prepared_bir_exposes_contract_sections` both passed.
