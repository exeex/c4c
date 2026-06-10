# Current Packet

Status: Complete
Source Idea Path: ideas/open/158_bir_comparison_condition_producer_identity.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Switch comparison and branch provenance consumers

## Just Finished

Step 5 completed the final direct prepared fused-compare operand producer read
switch inside `lower_missing_fused_compare_operand_publication`.

Missing fused-compare operand publication now uses
`bir::find_comparison_operand_producer` for the named operand at
`context.bir_block->insts.size()` to decide whether a same-block producer
instruction exists. When BIR reports a producer instruction, the existing
`hooks.emit_value_publication_to_register` path remains in use; otherwise the
prepared scalar publication plan/home fallback path is unchanged. The now-unused
local `find_prepared_fused_compare_operand_producer` wrapper was removed.

## Suggested Next

Supervisor can decide whether the completed Step 5 consumer switches are ready
for plan-owner review, the next plan step, or acceptance validation.

## Watchouts

- `lower_missing_fused_compare_operand_publication` no longer calls the
  prepared operand-producer lookup; do not restore that direct read in a later
  cleanup.
- Branch legality, condition-code selection, compare emission, branch emission,
  hazards, emitted-register state, final instruction policy, scratch/target
  choice, publication hooks, and the prepared scalar fallback path were left
  unchanged.
- This slice did not add or change test expectations.

## Proof

Delegated proof passed:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`.

`test_after.log` reports the build completed and backend CTest ran 179 tests
with 0 failures.
