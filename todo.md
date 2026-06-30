Status: Active
Source Idea Path: ideas/open/459_rv64_select_edge_suppression_placement_consumer.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route First Suppression Consumer Packet

# Current Packet

## Just Finished

Completed Step 3 implementation for idea 459. RV64 object emission now consumes
only explicit available `predecessor_edge_consumed_suppression` placement
metadata:

- added a shared prepared placement-record/move-bundle matcher;
- collected `PreparedSelectEdgeSourceProducerPlacementRecords` in the RV64
  prepared object route;
- exposed before-instruction register-destination bundles through traversal
  only when an available suppression placement record matches them;
- suppressed matching bundles as empty RV64 fragments;
- rejected any visible before-instruction register-destination bundle that
  reaches RV64 move-bundle lowering without suppression authority.

Focused RV64 tests cover accepted suppression, preservation of predecessor-edge
compare materialization, mismatched site/destination records publishing no
suppression and emitting no generic moves, and unsupported stack-destination
mutation rejecting through the existing RV64 move-bundle diagnostic.

Artifact:
`build/agent_state/459_step3_suppression_consumer/summary.md`.

## Suggested Next

Step 4: Residual Disposition And Close Readiness.

Re-probe or inspect the representative `20010329-1` route after the suppression
consumer. Confirm whether the target before-instruction
`predecessor_edge_consumed_suppression` owner is gone, classify any remaining
residual by first owner, and decide whether idea 459 is close-ready or needs one
exact follow-up packet.

## Watchouts

- Consume only explicit `predecessor_edge_consumed_suppression` placement
  metadata.
- RV64 currently has two consumer gaps that Step 3 must handle together:
  placement records are not collected in object emission, and prepared
  traversal does not expose the target register-destination before-instruction
  bundle.
- Do not infer suppression from value ids, block indexes, instruction indexes,
  raw BIR shape, filenames, function names, or one prepared dump.
- Do not add generic stack-to-register or register-to-register move support.
- Do not consume `load_from_stack_slot missing_stack_freshness`.
- Do not reopen idea 456 cast-dependency consumption.
- Step 3 intentionally did not expose unauthorized register-destination
  before-instruction bundles globally; doing so caused existing RV64 runtime
  object-route rows to fail on unrelated move bundles. Unauthorized suppression
  candidates remain unconsumed and are not lowered generically.
- Keep generic stack-home branch consumer work routed to
  `ideas/open/451_stack_home_branch_operand_materialization.md`.
- Keep pointer-value provenance and generic instruction-side lowering out of
  this plan.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, baseline logs, or
  `review/`.

## Proof

Step 3 implementation proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed. `test_after.log` reports all `^backend_` tests passed.
