# Semantic Instruction Result Frame-Slot Write Event Producer

Status: Open
Type: Lower-level prepared semantic instruction-result frame-slot write/materialization event producer idea
Parent: `ideas/closed/479_real_semantic_write_event_authority_producer.md`
Source Evidence: `build/agent_state/479_step4_real_event_authority_residual/`
Owning Layer: Prepared semantic instruction-result frame-slot write/materialization event production

## Goal

Produce durable prepared events proving that a semantic instruction result was
written or materialized into a specific frame slot.

## Why This Exists

Idea 479 could not populate real semantic write-event authority for
`%t23 = bir.ne i32 %t22, 0` into slot `#21`. Current prepared evidence has
semantic identity and destination facts, but no lower-level event source. The
existing `%t22 -> %t23` movement has `authority=none` and is storage movement,
not semantic compare-result materialization.

## In Scope

- Audit where semantic instruction results become frame-slot writes or
  materializations in prepared/prealloc data.
- Define a durable event producer for semantic instruction-result
  frame-slot write/materialization.
- Record function and stable event site.
- Record semantic producer identity, result value id/name/type, and opcode.
- Record event source/result matching the semantic result.
- Record destination slot/object/offset/size/alignment.
- Record authority proving semantic result materialization into the destination
  frame slot.
- Preserve fail-closed records for missing producer event, storage-only moves,
  source/result mismatch, destination mismatch, unsupported authority, and
  protected boundaries.

## Out Of Scope

- Consuming the event through idea 478/479 carrier availability.
- Path/dominance coverage, same-slot write exclusion, and effect non-clobber
  interval proof.
- Populating semantic interval, `PreparedFrameSlotSourceFact`, or
  `PreparedBranchStackLoadAuthority` rows.
- RV64 branch-load emission or target lowering.
- Treating storage-only moves such as `%t22 -> %t23` as semantic
  materialization when authority is absent.
- Pointer-value/provenance repair, select-result stack-destination repair, or
  unsupported-terminator branch-site relationship repair.
- Inferring event authority from raw BIR adjacency, final stack homes, storage,
  offsets, object ids, value names, function names, testcase names, or dump
  order.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.

## Acceptance Criteria

- Prepared semantic write/materialization events are produced only when a
  producer explicitly owns semantic result materialization into the destination
  frame slot.
- Positive coverage demonstrates at least one scalar instruction-result event
  with function/site, semantic result identity, destination slot/object/layout,
  and explicit authority.
- Negative coverage rejects raw BIR/branch-condition-only evidence, final
  home/storage/object-only evidence, `authority=none` storage moves,
  source/result mismatches, destination mismatches, unsupported authority,
  protected boundaries, and raw-shape fixtures.
- Fresh residual disposition states whether idea 479-style real event-authority
  population may resume or whether another event producer remains first.

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
