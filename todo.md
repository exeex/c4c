# Current Packet

Status: Active
Source Idea Path: ideas/open/705_prepared_fact_boundary_from_bir_views.md
Source Plan Path: plan.md
Current Step ID: 2.3b.1
Current Step Title: Produce independent named current-block evidence in prealloc

## Just Finished

- Step 2.3b.1 — added a prealloc-owned current-block evidence adapter that
  queries `BirProducerView` independently of prepared publication output, plus
  a unique function/block/instruction/value selector reused by the prepared
  current-block attachment.
- The prepared query can generate this evidence directly from an explicit BIR
  function input; missing, incomplete, ambiguous, and mismatched selection is
  fail closed, while the existing prepared-only non-PHI `JoinTransfer` path is
  unchanged.

## Suggested Next

- Execute Step 2.3b.2 to publish uniquely edge-bound prepared incoming/source
  routing authority and add registered adjacent-edge collision proof.

## Watchouts

- Keep the Step 2.3b.1 adapter independent: do not replace its named BIR query
  with identity copied from prepared publication output.
- Step 2.3b.2 must bind routing to one predecessor/successor/destination edge;
  do not add function-wide publication or `JoinTransfer` scans in AArch64.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- Result: build passed; all 309/309 backend tests passed. The registered
  `backend_prepared_fact_boundary_contract` covers adapter production plus
  positive and missing/incomplete/ambiguous/mismatched selection. Canonical
  proof log: `test_after.log`.
