# Real Semantic Write Event Authority Producer

Status: Open
Type: Prepared real semantic instruction-result write/materialization event authority producer idea
Parent: `ideas/closed/478_semantic_instruction_result_write_event_carrier.md`
Source Evidence: `build/agent_state/478_step4_semantic_write_event_carrier_residual/`
Owning Layer: Real prepared semantic write/materialization event authority

## Goal

Populate real authoritative semantic instruction-result frame-slot
write/materialization event carriers for rows such as `%t23 = ne i32 %t22, 0`
into slot `#21`.

## Why This Exists

Idea 478 completed the independent event carrier/status surface. Real `%t23`
availability is still blocked because current prepared evidence has destination
and semantic identity facts but no durable producer that supplies explicit
semantic write-event authority. Storage-only moves with `authority=none` remain
rejected.

## In Scope

- Audit current instruction-result, publication, move, value-home, and
  frame-slot evidence for real semantic write-event authority.
- Produce authoritative semantic write/materialization events only when the
  producer can prove the semantic result was written to the destination frame
  slot/object.
- Populate event site/source, semantic result id/name/type/opcode, destination
  slot/object/offset/size/alignment, and event authority.
- Preserve fail-closed statuses for missing authority, storage-only moves,
  semantic result mismatch, destination mismatch, unsupported authority, and
  protected boundaries.
- Provide focused positive and fail-closed coverage for real scalar
  instruction-result frame-slot write authority.

## Out Of Scope

- Defining another carrier/status API unless Step 1 proves idea 478's surface
  is structurally insufficient.
- Path/dominance coverage, same-slot write exclusion, and effect non-clobber
  interval proof.
- Populating real semantic materialization interval records as available.
- Populating `PreparedFrameSlotSourceFact` or `PreparedBranchStackLoadAuthority`
  rows.
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

- Real semantic write-event authority is populated only from explicit producer
  evidence, not from final homes, raw shape, names, or storage movement.
- Positive coverage demonstrates at least one real scalar instruction-result
  frame-slot write event with coherent semantic result identity, destination,
  and explicit authority.
- Negative coverage rejects missing event authority, storage-only moves,
  semantic result mismatch, destination mismatch, unsupported authority,
  protected boundaries, and raw-shape fixtures.
- Fresh residual disposition states whether real semantic interval population
  may resume or whether another lower-level producer remains first.

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
