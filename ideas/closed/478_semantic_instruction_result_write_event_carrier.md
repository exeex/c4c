# Semantic Instruction Result Write Event Carrier

Status: Closed
Type: Prepared semantic instruction-result frame-slot write/materialization event carrier idea
Parent: `ideas/closed/477_real_semantic_materialization_interval_fact_population.md`
Source Evidence: `build/agent_state/477_step4_real_semantic_fact_population_residual/`
Owning Layer: Prepared semantic write/materialization event carriers

## Goal

Publish authoritative prepared event carriers for semantic instruction-result
frame-slot write/materialization events.

## Completion Notes

Closed after Step 4 residual disposition as complete for the independent
semantic instruction-result frame-slot write/materialization carrier/status
surface.

Step 3 added:

- `PreparedSemanticWriteEventAuthorityKind`;
- `PreparedSemanticWriteEventCarrierStatus`;
- `PreparedSemanticWriteEventCarrierInputs`;
- `PreparedSemanticWriteEventCarrier`;
- `plan_prepared_semantic_write_event_carrier`;
- `prepared_semantic_write_event_carrier_available`.

The completed surface accepts explicit semantic-result write authority and
matching destination facts, and rejects missing semantic identity, missing
event authority, storage-only movement, semantic result mismatches,
destination mismatches, unsupported authority, and protected boundary rows.

Representative residual:

- Real `%t23 = ne i32 %t22, 0` into slot `#21` still lacks explicit semantic
  write-event authority; destination/home/object evidence alone remains
  `missing_event_authority`.
- `%t22 -> %t23` with `authority=none` is classified as storage-only movement
  and remains rejected as semantic compare-result materialization.
- Path/no-clobber interval proof, source-fact population, branch-stack-load
  authority, and RV64 lowering remain downstream and blocked.

Follow-up source idea:
`ideas/open/479_real_semantic_write_event_authority_producer.md`.

## Reviewer Reject Signals

- Reject RV64 lowering changes presented as semantic write-event carrier work.
- Reject directly setting downstream semantic interval, source-fact, or
  branch-stack-load availability from this idea.
- Reject treating `%t22 -> %t23` or other storage-only moves as semantic
  instruction-result materialization when authority is absent.
- Reject inferring write-event authority from raw BIR shape, final stack homes,
  storage, offsets, object ids, value names, function names, testcase names, or
  dump order.
- Reject folding path/no-clobber interval proof, pointer/provenance,
  select-result stack-destination, or unsupported-terminator repair into this
  event-carrier idea.
- Reject expectation rewrites, unsupported downgrades, allowlists, baseline
  churn, or classification-only changes claimed as capability progress.
