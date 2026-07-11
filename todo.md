Status: Active
Source Idea Path: ideas/open/701_route_fact_test_dump_contract_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Rewrite or gate the selected route vocabulary

# Current Packet

## Just Finished

Step 1 selected one eligible route vocabulary family for cleanup:
Route 4 block-entry publication attribution/printer vocabulary.

Selected family: Route 4 block-entry publication compatibility wording around
prepared current-block-entry publication lookup and prepared-printer agreement.
This family is eligible because the named proof surface already exists above
route dumps:

- `backend_prealloc_block_entry_publications` exercises
  `PreparedCurrentBlockEntryPublication`,
  `PreparedBlockEntryPublication`, and
  `find_bir_block_entry_publication_identity()` positive and fail-closed
  behavior, including agreeing, missing, mismatched, wrong-successor,
  wrong-key, duplicate, and prepared-fallback cases.
- `backend_cli_dump_prepared_bir_exposes_contract_sections` already checks the
  prepared dump section `--- prepared-block-entry-publications ---` and named
  `block_entry_publication ...` rows.

Why this is not route-dump-only proof: the selected surface is a prepared
contract/lookup proof plus prepared dump contract coverage. Route 4 evidence
remains compatibility attribution only; executable or prepared availability is
not authorized by the route label.

Expected files for the next packet:

- `src/backend/prealloc/value_locations.hpp` if route-numbered field names need
  named compatibility aliases or comments.
- `src/backend/prealloc/prepared_lookups.cpp` for Route 4 block-entry
  attribution helper/vocabulary cleanup if needed.
- `src/backend/prealloc/prepared_printer/value_locations.cpp` for prepared
  block-entry publication printer wording if needed.
- `tests/backend/bir/backend_prealloc_block_entry_publications_test.cpp` for
  test assertion vocabulary cleanup.
- `todo.md` and `test_after.log`.

## Suggested Next

Delegate Step 2 to rewrite or gate the selected Route 4 block-entry
publication attribution/printer vocabulary behind the named prepared
block-entry publication proof surface. Keep the slice narrow to this selected
family.

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

## Proof

No build/test proof required for this todo-only selection packet.

Recommended supervisor proof command for the next code packet:

```sh
cmake --preset default -DC4C_ENABLE_PREPARED_FACT_TESTS=ON && cmake --build --preset default --target backend_prealloc_block_entry_publications_test c4cll && ctest --test-dir build -R '^(backend_prealloc_block_entry_publications|backend_cli_dump_prepared_bir_exposes_contract_sections)$' --output-on-failure | tee test_after.log
```
