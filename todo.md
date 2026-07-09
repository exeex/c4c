Status: Active
Source Idea Path: ideas/open/635_prepared_branch_stack_clobber_safety_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Trace Prepared Clobber-Safety Authority

# Current Packet

## Just Finished

Step 2 traced the prepared clobber-safety authority boundary for the seven Step
1 selected branch stack-load pointer rows without implementation edits.

Exact producer functions:

- `collect_prepared_branch_stack_load_authorities` walks prepared control-flow
  branch conditions, finds the BIR block, and sets
  `branch_terminator_instruction_index = block->insts.size()`.
- `collect_branch_stack_load_authority_for_role` limits records to named
  stack-slot operands for `Condition`, `Lhs`, and `Rhs`.
- `make_branch_stack_load_authority_record` constructs
  `PreparedBranchStackLoadAuthorityRecord`, publishes the selected freshness
  candidate with `publish_prepared_branch_stack_source_freshness_candidate`,
  proves pointer identity with
  `prepared_branch_stack_load_pointer_operand_is_proven`, chooses
  `policy`/`pointer_status`, and passes clobber safety from
  `prepared_collected_branch_stack_load_clobber_safe`.
- `plan_prepared_branch_stack_load_authority` verifies names, terminator,
  branch condition, target labels, selected source freshness, selected source
  identity, clobber-safety input, and pointer status before setting
  `Available`.

Carrier fields already present: `PreparedBranchStackLoadAuthority::role`,
`policy`, `pointer_status`, `value_id`, `value_name`, `slot_id`,
`stack_object_id`, `branch_block_index`, `branch_terminator_instruction_index`,
`stack_slot_fresh_at_branch`, `source_freshness_authorities`,
`source_freshness_status`, and `source_freshness_authority`. The input-only
field `PreparedBranchStackLoadAuthorityInputs::stack_slot_clobber_safe_at_branch`
is consumed but no detailed clobber-safety fact, proof kind, or fail-closed
clobber reason is persisted in the branch authority.

First missing boundary: `prepared_collected_branch_stack_load_clobber_safe` is a
placeholder, not a real intervening-clobber analysis. For pointer `Lhs`/`Rhs`,
it requires `pointer_operand_proven` and
`branch_stack_load_has_no_intervening_instructions`, which only returns true
when `branch_terminator_instruction_index == 0`. All seven selected rows have
terminator indices greater than zero, so they fail closed as
`MissingStackClobberSafety` before RV64 can consume them.

Consumer checks before RV64 emission: `selected_branch_stack_load_source_freshness_status`
matches role, block label, value id/name, branch block index, and terminator
instruction index, requires `prepared_branch_stack_load_authority_available`,
and rechecks the selected freshness authority fields before
`fragment_for_prepared_fused_pointer_branch` emits. RV64 currently does not
infer safety from stack homes, offsets, or final assembly shape.

Fail-closed states found or required:

- missing selected freshness: existing `MissingSourceFreshnessAuthority`
- stale/contradictory freshness: existing `InvalidSourceFreshnessAuthority`,
  `AmbiguousSourceFreshnessAuthority`, or `UnsupportedSourceFreshnessAuthority`
- missing clobber proof: existing `MissingStackClobberSafety`
- mismatched branch source identity: existing selected-freshness match rejects
  wrong value, home, role, block, or terminator index
- contradictory clobber proof: not yet branch-specific; the adjacent
  `PreparedFrameSlotSourceFact` carrier has same-slot write, call/helper,
  publication, move-bundle, and parallel-copy clobber statuses, but branch
  stack-load authority does not consume those facts or equivalent analysis yet

Evidence artifacts:

- `build/agent_state/635_step2_authority_boundary_summary.md`
- `build/agent_state/635_step2_branch_stack_authority_producer_excerpt.txt`
- `build/agent_state/635_step2_branch_stack_authority_carrier_excerpt.txt`
- `build/agent_state/635_step2_frame_slot_fact_boundary_excerpt.txt`
- `build/agent_state/635_step2_frame_slot_fact_carrier_excerpt.txt`
- `build/agent_state/635_step2_rv64_branch_consumer_excerpt.txt`
- `build/agent_state/635_step2_make_branch_stack_load_authority_record_callees.json`
- `build/agent_state/635_step2_plan_prepared_branch_stack_load_authority_callees.json`
- `build/agent_state/635_step2_selected_branch_stack_load_source_freshness_status_callees.json`
- `build/agent_state/635_step2_635_step1_*_authority_trace.txt`

## Suggested Next

Smallest non-overfit Step 3 packet: implement a prepared-layer clobber-safety
proof for branch stack-slot pointer operands in the branch authority producer
path. Replace the `terminator_instruction_index == 0` placeholder with a
general branch-use-point analysis bound to the selected `BranchStackLoadSource`
authority, exact stack slot/object, and exact branch block/terminator index.
Preserve fail-closed rejection for absent selected freshness, stale/mismatched
freshness, unknown path validity, same-slot writes, call/helper clobbers,
publication clobbers, move-bundle clobbers, and parallel-copy clobbers. Do not
edit RV64 admission first and do not special-case source files, functions,
roles, or offsets.

## Watchouts

`PreparedFrameSlotSourceFact` is useful precedent, but Step 3 should not simply
reuse it unless the proof binds the selected branch stack source to the exact
branch use point. A consumer-only RV64 patch would be route drift because the
prepared authority remains `MissingStackClobberSafety`. A boolean-only fix that
marks all intervening-instruction pointer rows safe would also overfit; the
repair needs distinguishable fail-closed clobber outcomes.

## Proof

Trace-only proof packet. No code build/test was delegated and no
`test_after.log` was written. Read-only evidence was captured in
`build/agent_state/635_step2_*`, including targeted source excerpts,
AST-backed callee maps from `c4c-clang-tool-ccdb`, and per-row prepared dump
authority traces.
