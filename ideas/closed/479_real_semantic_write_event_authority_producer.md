# Real Semantic Write Event Authority Producer

Status: Closed
Type: Prepared real semantic instruction-result write/materialization event authority producer idea
Parent: `ideas/closed/478_semantic_instruction_result_write_event_carrier.md`
Source Evidence: `build/agent_state/478_step4_semantic_write_event_carrier_residual/`
Owning Layer: Real prepared semantic write/materialization event authority

## Goal

Populate real authoritative semantic instruction-result frame-slot
write/materialization event carriers for rows such as `%t23 = ne i32 %t22, 0`
into slot `#21`.

## Completion Notes

Closed after Step 4 residual disposition as a routed blocker. Idea 479 did not
complete real event-authority population from current prepared evidence.

The completed findings are:

- The idea 478 carrier/status API is sufficient and is not the blocker.
- Real `%t23 = bir.ne i32 %t22, 0` semantic identity is present.
- Real `%t23` destination facts are present: value id `17`, slot `#21`, object
  `#21`, offset `156`, size `4`, align `4`.
- No durable event source proves semantic instruction-result materialization
  into slot `#21`.
- `%t22 -> %t23` has `authority=none` and remains rejected as storage-only
  movement, not semantic event authority.
- Raw BIR, branch condition, final home, storage, object, and slot layout are
  visible but insufficient.
- Path/no-clobber interval proof, source-fact population, branch-stack-load
  authority, and RV64 lowering remain downstream and blocked.

Follow-up source idea:
`ideas/open/480_semantic_instruction_result_frame_slot_write_event_producer.md`.

## Reviewer Reject Signals

- Reject RV64 lowering changes presented as real semantic event-authority
  producer work.
- Reject directly setting downstream semantic interval, source-fact, or
  branch-stack-load availability from this idea.
- Reject treating `%t22 -> %t23` or other storage-only moves as semantic
  instruction-result materialization when authority is absent.
- Reject inferring event authority from raw BIR shape, final stack homes,
  storage, offsets, object ids, value names, function names, testcase names, or
  dump order.
- Reject folding path/no-clobber interval proof, pointer/provenance,
  select-result stack-destination, or unsupported-terminator repair into this
  event-authority producer.
- Reject expectation rewrites, unsupported downgrades, allowlists, baseline
  churn, or classification-only changes claimed as capability progress.
