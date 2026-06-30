# Before-Instruction Stack-To-Register Move Materialization

Status: Open
Type: RV64 prepared move-bundle materialization idea
Parent: `ideas/closed/456_rv64_select_edge_cast_dependency_consumer.md`
Source Evidence: `build/agent_state/456_step7_final_residual_disposition/`
Owning Layer: RV64 before-instruction move-bundle materialization for ordinary
register-destination consumers

## Goal

Classify and implement, if justified, the next before-instruction move-bundle
materialization family exposed after idea 456: ordinary stack/register-source
copies into register destinations, especially the `20010329-1`
`consumer_stack_to_register` bundle at `block_index=4 instruction_index=2`.

## Why This Exists

Idea 456 completed explicit cast-dependency rematerialization and suppression
of the authorized stack publication for `%t17`. After that work, `20010329-1`
still fails with `unsupported_move_bundle_target_shape`. Step 7 found the next
owner is a later before-instruction move bundle containing register-destination
moves, including `from_value_id=9 to_value_id=10 destination_storage=register
reason=consumer_stack_to_register`. That is not an explicit cast-dependency
consumer gap; it is a separate move-bundle/materialization family.

## In Scope

- Audit the `block_index=4 instruction_index=2` before-instruction move bundle
  and nearby prepared facts in `20010329-1`.
- Classify each move source and destination home, including register-source
  and stack-source operands, destination register constraints, carrier use,
  scratch/clobber hazards, and current traversal event exposure.
- Define the RV64 materialization contract for accepted before-instruction
  stack-to-register or register-to-register moves in this family.
- Add focused RV64 object coverage for accepted materialization and fail-closed
  unsupported, stale, ambiguous, clobbering, or policy-missing cases.
- Re-probe `20010329-1` and route any remaining residuals separately.

## Out Of Scope

- Reopening explicit cast-dependency authority consumption closed by idea 456.
- Consuming `load_from_stack_slot` dependency-operand authority with
  `missing_stack_freshness`.
- Generic stack-home branch operand/condition materialization, tracked by
  `ideas/open/451_stack_home_branch_operand_materialization.md`.
- Pointer-value memory provenance publication, local/global store publication,
  and generic instruction-side lowering.
- Inferring move authority from raw BIR shape, filenames, function names,
  block names, testcase names, or one prepared dump.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, or `test_baseline.new.log` churn.

## Acceptance Criteria

- The `block_index=4 instruction_index=2` before-instruction move bundle is
  classified with exact accepted materialization authority or exact
  fail-closed reason.
- Accepted stack-to-register or register-to-register moves consume explicit
  prepared move-bundle facts and preserve scratch/clobber safety.
- Missing, stale, ambiguous, unsupported, or unrelated move bundles remain
  fail-closed with focused coverage.
- Remaining non-move-bundle residuals are routed separately instead of being
  folded into this idea.

## Reviewer Reject Signals

- Reject broad generic stack move support that is not tied to explicit
  prepared before-instruction move-bundle facts.
- Reject testcase-shaped fixes keyed to `20010329-1`, block index 4,
  instruction index 2, value ids 7/9/10, one stack slot, or one prepared dump
  layout.
- Reject weakening idea 456's cast-dependency guards or consuming
  `load_from_stack_slot missing_stack_freshness` as part of this route.
- Reject expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, or baseline/log churn.
