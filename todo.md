# Current Packet

Status: Active
Source Idea Path: ideas/open/707_prepared_stack_destination_authority_positive_gate.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Establish one real stack-backed publication/move producer

## Just Finished

- Audited the rejected Plan Step 2 slice against
  `review/idea707_step2_positive_producer_review.md` with AST-backed caller and
  type-reference queries. The selected collected `Lhs` branch-stack-load
  fixture has no real `PreparedEdgePublication` and no bound
  `PreparedMoveResolution`: its collector only consumes branch authority, and
  the fixture creates neither parallel-copy edge publication state nor a real
  destination move relationship.
- Removed the synthetic destination/source equality, home-kind constants,
  load-policy-as-move substitution, nominal status surface, mirrored tests,
  and test-order workaround. All implementation/test files are restored to the
  Step 1 state; no false authority row remains to advance into MIR.

## Suggested Next

- Execute Plan Step 2.1. Start from the out-of-SSA parallel-copy/select producer
  family around `check_select_edge_source_producer_placement_contract`, which
  already has a real predecessor/successor edge and bound
  `PreparedMoveResolution`, then establish or select a semantic variant with a
  genuinely stack-backed destination and independently owned applicable
  branch/aggregate evidence.
- Prove the upstream relationship green before defining composer inputs in
  Step 2.2. Do not compose or expose a
  `PreparedStackDestinationAuthorityView` row during Step 2.1.

## Watchouts

- The original Step 2 route was blocked on the selected fixture's missing real
  publication/move relationship. The repaired runbook splits producer
  establishment (2.1), reachable composer inputs (2.2), and row composition
  (2.3); do not skip ahead or manufacture authority from branch-load
  policy/freshness classifications.
- Preserve the reviewer acceptance conditions in
  `review/idea707_step2_positive_producer_review.md`: exact publication/edge
  identity, real bound move, prepared-owned homes, independently reachable
  negative states, and a genuinely green focused proof.
- The existing backend test still has the pre-existing global-publication
  failure. No genuinely green focused Step 2 contract exists because the
  required real producer path could not be established.

## Proof

- After removing the rejected slice, ran exactly
  `(cmake --build build --target backend_prepare_stack_layout_test -j && ctest
  --test-dir build -R '^backend_prepare_stack_layout$' --output-on-failure)
  2>&1 | tee test_after.log`.
- Fresh compile/link succeeded. CTest reproduces the unchanged `test_before.log`
  failure: `expected coherent global store source publication authority`.
- No green acceptance proof was claimed. Proof log: `test_after.log`.
