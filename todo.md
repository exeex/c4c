# Current Packet

Status: Active
Source Idea Path: ideas/open/705_prepared_fact_boundary_from_bir_views.md
Source Plan Path: plan.md
Current Step ID: 2.3a
Current Step Title: Contract publication selection at the prealloc boundary

## Just Finished

- Step 2.3a — confirmed current-block publication selection uses applicable
  named producer evidence plus prepared publication, home, move, freshness,
  and control authority while Route 5 compatibility fields remain
  observational only.
- Strengthened prepared-originated non-PHI `JoinTransfer` authority selection
  to reject duplicate semantic transfer candidates and added focused ambiguous
  prepared-authority proof alongside the existing available and fail-closed
  named-evidence coverage.

## Suggested Next

- Execute Step 2.3b to retire the bounded Route 5 public compatibility payload
  and migrate only its known AArch64 dispatch, printer, and contract-test
  consumers to the Step 2.3a boundary.

## Watchouts

- Step 2.3b must keep the AArch64 change to the existing compatibility consumer;
  do not migrate unrelated target materializers or replan publication authority
  in the target.
- Preserve the non-PHI prepared `JoinTransfer` path without synthesizing a BIR
  CFG-edge publication relation.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- Result: build passed; all 309/309 backend tests passed. Canonical proof log:
  `test_after.log`.
