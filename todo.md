Status: Active
Source Idea Path: ideas/open/227_phase_e3_branch_target_helper_oracle_follow_up.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Selected-Row Fallback And Nearby Same-Feature Proof

# Current Packet

## Just Finished

Completed Step 3 - Add Selected-Row Fallback And Nearby Same-Feature Proof.

Added focused selected-row AArch64 consumer proof in
`tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp` for the
materialized-bool conditional branch row:

- factored the existing selected materialized-bool branch assertions into a
  reusable helper that checks the emitted branch payload, selected instruction
  target, typed condition operand, and prepared true/false block successors
- added raw BIR `true_label`/`false_label` text drift proof showing agreeing
  structured ids preserve the prepared branch payload and successors
- added fallback matrix coverage showing prepared branch payloads and
  successors are retained when BIR structured successor ids are invalid,
  mismatched, or conflicting
- kept nearby same-feature proof mapped to existing fused-compare,
  stale-lookup, loop-header divergent-id, and prepared-branch-record tests in
  the delegated proof subset

Consumer-level non-conditional BIR fallback proof is unreachable through
`dispatch_prepared_block(...)` and
`lower_prepared_conditional_branch_terminator(...)`: both reject a
non-conditional BIR terminator before the selected row is reachable. That
boundary remains covered at helper-reader level by
`backend_prepared_lookup_helper_test.cpp`.

## Suggested Next

Supervisor should review and commit the completed Step 3 test/proof slice, then
decide whether the active runbook needs lifecycle closure, another execution
packet, or plan-owner review.

## Watchouts

- Keep the slice to the non-compare/materialized-bool conditional branch row;
  do not retarget fused-compare, materialized compare join, unconditional
  branch, wrapper, printer, branch spelling, edge-copy, target-policy, or
  emitted-output surfaces.
- The private reader is fail-closed and has no duplicate-row table; treat
  duplicate/conflict proof as conflicting structured ids unless the supervisor
  requests a separate indexed-context design.
The selected-row consumer proof intentionally does not include non-conditional
BIR because the public consumer APIs reject that shape before the agreement
fallback row is reachable. Keep that as helper-reader coverage unless a future
plan explicitly changes the consumer API boundary.

## Proof

Green delegated proof written to `test_after.log`:

`bash -o pipefail -c "cmake --build build --target backend_prepared_lookup_helper_test backend_aarch64_branch_control_lowering_test backend_aarch64_prepared_branch_records_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_branch_control_lowering|backend_aarch64_prepared_branch_records)$' --output-on-failure" > test_after.log 2>&1`
