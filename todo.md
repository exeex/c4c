# Current Packet

Status: Active
Source Idea Path: ideas/open/718_prepared_routing_root_dependency_classification_decomposition.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Prove unchanged integration and hand back

## Just Finished

- Step 8 completed the unchanged integration handback. Generic owner
  preparation now preserves legitimate BIR-PHI-root incoming dependencies when
  the prepared publication/move pair does not cover that stable key, while
  retaining duplicate roots for ambiguity-safe fail-closed consumption.
- Resolved the Step 8 review findings by making prepared-root suppression
  require applicable predecessor/successor, destination, routed stable key,
  available status, and a known semantic origin; exact negative prepared roots
  now propagate an incomplete routing fact instead of being bypassed.
- Added focused owner matrices for uncovered BIR-PHI roots (positive, missing
  predecessor, and duplicate ambiguity) and Source-role agreement (different
  predecessor/source positive; destination, origin, and duplicate negatives).
- Registered `backend_current_block_phi_root_authority_probe` in the default
  CTest graph. It also covers parallel uncovered roots with differing
  destinations and exact-negative prepared-root propagation to `Incomplete`.
- AArch64 consumes only attached stable-key owner queries. The unchanged
  policy-present routing vectors and short-circuit dispatch contract pass; the
  original `%rhs.add` and `%short.selected` producers remain intact.

## Suggested Next

- Hand the completed Step 8 slice back to the supervisor for review, regression
  log normalization, and commit handling.

## Watchouts

- BIR-PHI-root facts are emitted only for uncovered stable keys. Existing
  prepared routing remains authoritative only when its complete applicable
  dependency identity agrees; multiple uncovered PHI roots are deliberately
  retained so the all-applicable query reports ambiguity.
- No AArch64 reconstruction fallback, fixture-name matcher, expectation
  downgrade, or producer rewrite was introduced.

## Proof

- Ran `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' > test_after.log 2>&1` exactly as
  delegated. Build passed and all 329/329 backend tests passed, including the
  default-registered PHI-root authority probe. Proof log:
  `test_after.log`.
- The acceptance matrices now live in the default-registered
  `backend_current_block_phi_root_authority_probe`; redundant copies were
  removed from the normally disabled prepared-lookup helper.
