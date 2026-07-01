# Semantic Instruction Result Write Event Carrier

Status: Open
Type: Prepared semantic instruction-result frame-slot write/materialization event carrier idea
Parent: `ideas/closed/477_real_semantic_materialization_interval_fact_population.md`
Source Evidence: `build/agent_state/477_step4_real_semantic_fact_population_residual/`
Owning Layer: Prepared semantic write/materialization event carriers

## Goal

Publish authoritative prepared event carriers for semantic instruction-result
frame-slot write/materialization events.

## Why This Exists

Idea 477 could not populate real semantic materialization interval facts for
`%t23 = ne i32 %t22, 0` because no durable prepared event carrier proves that
the semantic result was written or materialized into slot `#21`. Existing
storage movement such as `%t22 -> %t23` has `authority=none` and must not be
treated as semantic compare-result materialization.

## In Scope

- Audit current prepared event/move/publication surfaces for possible semantic
  instruction-result write event producers.
- Define an authoritative event carrier with function, event site, semantic
  producer identity, result value id/name/type, opcode/kind, destination frame
  slot/object/offset/size/alignment, and event authority.
- Populate event carriers only when producer evidence proves semantic
  instruction-result materialization, not storage movement.
- Preserve status detail for missing event, storage-only move, destination
  mismatch, unsupported boundary, and unknown authority.
- Provide focused positive and fail-closed coverage for scalar instruction
  result frame-slot writes.

## Out Of Scope

- Path/dominance coverage from event to consumer.
- Same-slot write exclusion and call/helper/inline-asm effect safety.
- Publication, move-bundle, and parallel-copy non-clobber interval
  classification beyond identifying storage-only moves as non-authoritative.
- Directly populating real semantic materialization interval records as
  available.
- Directly populating `PreparedFrameSlotSourceFact` or
  `PreparedBranchStackLoadAuthority` rows.
- RV64 branch-load emission or target lowering.
- Reusing `%t22 -> %t23` or other storage-only moves as semantic
  materialization when authority is absent.
- Pointer-value/provenance repair, select-result stack-destination repair, or
  unsupported-terminator branch-site relationship repair.
- Inferring event authority from raw BIR adjacency, final stack homes, storage,
  offsets, object ids, value names, function names, testcase names, or dump
  order.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.

## Acceptance Criteria

- A prepared event carrier can represent authoritative semantic
  instruction-result frame-slot write/materialization events.
- Positive coverage proves at least one scalar instruction-result write event
  only when semantic result identity, destination frame-slot/object identity,
  and event authority are explicit.
- Negative coverage rejects missing event authority, storage-only moves,
  semantic result mismatch, destination mismatch, pointer/provenance gaps,
  select-result stack destinations, unsupported terminators, and raw-shape
  fixtures.
- Fresh residual disposition states whether idea 477-style real semantic
  interval population may resume or whether another lower-level event owner
  remains first.

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
