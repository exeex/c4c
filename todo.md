# Current Packet

Status: Active
Source Idea Path: ideas/open/720_block_entry_publication_proof_evidence_multiplicity.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add focused internal proof rows

## Just Finished

- Completed Plan Step 3 by adding four focused internal backend BIR proof rows
  that directly construct exact owner/value destination identity,
  independently attributed duplicate claims, stale instruction coordinates,
  and missing attribution.
- Kept each row scoped to one primary preservation contract, documented why
  source programs cannot express the malformed/internal state, and added no
  classification or production behavior.

## Suggested Next

- Execute Plan Step 4: make Route4 the authoritative classifier for the proof
  facts preserved by the Step 3 rows.

## Watchouts

- The proof rows intentionally inspect the Step 2 data contracts directly;
  Step 4 remains responsible for all equality and classification policy.
- Exact identity must use owner/value pointers rather than diagnostic name/type,
  and agreeing claims with distinct attribution IDs must remain independently
  observable.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_prepared_lookup_helper$' | tee
  test_after.log` passed (1/1); `test_after.log` is the proof log.
