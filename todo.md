# Current Packet

Status: Active
Source Idea Path: ideas/open/720_block_entry_publication_proof_evidence_multiplicity.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add focused internal proof rows

## Just Finished

- Completed Plan Step 2 by defining Route4-owned typed contracts for exact
  destination identity, independently attributed coordinate-bearing claims,
  an order- and multiplicity-preserving claim collection, and a classification
  result covering unavailable, available, missing, unattributed, stale,
  inconsistent, and ambiguous evidence.
- Kept display name/type as diagnostic metadata and left all existing Route4,
  prepared, and MIR APIs and consumers unchanged.

## Suggested Next

- Execute Plan Step 3: add focused internal proof rows that construct the new
  collection and distinguish exact destination identity, independently
  attributed duplicates, stale coordinates, and missing attribution.

## Watchouts

- The new contracts intentionally contain no equality or classification helper;
  Step 4 remains the authority migration and must use owner/value identity, not
  the diagnostic name/type fields.
- Preserve claim vector order and multiplicity in Step 3; agreeing claims with
  distinct `attribution_id` values must remain independently observable.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_prepared_lookup_helper$' | tee
  test_after.log` passed (1/1); `test_after.log` is the proof log.
