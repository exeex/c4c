# Semantic Instruction Result Frame-Slot Write Event Producer

Status: Closed
Type: Lower-level prepared semantic instruction-result frame-slot write/materialization event producer idea
Parent: `ideas/closed/479_real_semantic_write_event_authority_producer.md`
Source Evidence: `build/agent_state/479_step4_real_event_authority_residual/`
Owning Layer: Prepared semantic instruction-result frame-slot write/materialization event production

## Goal

Produce durable prepared events proving that a semantic instruction result was
written or materialized into a specific frame slot.

## Completion Notes

Closed after Step 4 residual disposition as a routed blocker. Idea 480 did not
identify a sound implementation packet from current prepared/prealloc evidence.

The completed findings are:

- `%t23 = bir.ne i32 %t22, 0` semantic identity is present.
- `%t23` final destination is present: value id `17`, home slot `#21`,
  storage `slot#21+stack156`, object `#21`, offset `156`, size `4`, align `4`.
- No durable event site/source proves `%t23` was written or materialized into
  slot `#21`.
- `%t22 -> %t23` has `authority=none`, `from_value_id=16`, `to_value_id=17`,
  and is rejected as storage-only movement with the wrong semantic source.
- Raw BIR, branch condition, final home, storage, object, and slot layout are
  visible but insufficient.
- Event-authority consumption, path/no-clobber interval proof, source facts,
  branch-stack-load authority, and RV64 lowering remain downstream and
  blocked.

Follow-up source idea:
`ideas/open/481_semantic_result_frame_slot_materialization_point_producer.md`.

## Reviewer Reject Signals

- Reject RV64 lowering changes presented as semantic write-event producer work.
- Reject directly setting downstream semantic event-authority, interval,
  source-fact, or branch-stack-load availability from this idea.
- Reject treating `%t22 -> %t23` or other storage-only moves as semantic
  instruction-result materialization when authority is absent.
- Reject inferring event authority from raw BIR shape, final stack homes,
  storage, offsets, object ids, value names, function names, testcase names, or
  dump order.
- Reject folding path/no-clobber interval proof, pointer/provenance,
  select-result stack-destination, or unsupported-terminator repair into this
  lower-level event producer.
- Reject expectation rewrites, unsupported downgrades, allowlists, baseline
  churn, or classification-only changes claimed as capability progress.
