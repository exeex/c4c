# Real Semantic Materialization Interval Fact Population

Status: Closed
Type: Prepared semantic materialization/write and interval fact population idea
Parent: `ideas/closed/476_semantic_instruction_result_frame_slot_materialization_facts.md`
Source Evidence: `build/agent_state/476_step4_semantic_materialization_interval_residual/`
Owning Layer: Real prepared semantic materialization/write and no-clobber interval facts

## Goal

Populate real prepared semantic instruction-result frame-slot
materialization/write records and path/no-clobber interval facts for scalar
branch stack slots.

## Completion Notes

Closed after Step 4 residual disposition as a routed blocker. Idea 477 did not
complete real semantic fact population for `%t23`; it identified the first
missing lower-level owner.

The completed findings are:

- Real `%t23 = ne i32 %t22, 0` identity is visible.
- Destination facts are visible: value id `17`, slot `#21`, stack offset
  `156`, object `#21`, type `i32`.
- No durable prepared event carrier proves `%t23` was written/materialized into
  slot `#21`.
- `%t22 -> %t23` has `authority=none`, `from_value_id=16`, `to_value_id=17`,
  and is rejected as storage movement rather than semantic materialization.
- Path/no-clobber interval facts remain missing after an event carrier exists.
- Source-fact population, branch-stack-load authority, and RV64 lowering remain
  downstream and blocked.

Follow-up source idea:
`ideas/open/478_semantic_instruction_result_write_event_carrier.md`.

## Reviewer Reject Signals

- Reject RV64 lowering changes presented as semantic fact population work.
- Reject directly setting downstream source-fact or branch-stack-load
  availability from this idea.
- Reject treating `%t22 -> %t23` or other storage-only moves as semantic
  instruction-result materialization when authority is absent.
- Reject inferring materialization/no-clobber facts from raw BIR shape, final
  stack homes, storage, offsets, object ids, value names, function names,
  testcase names, or dump order.
- Reject folding pointer-value/provenance, select-result stack-destination, or
  unsupported-terminator repair into this semantic fact population idea.
- Reject expectation rewrites, unsupported downgrades, allowlists, baseline
  churn, or classification-only changes claimed as capability progress.
