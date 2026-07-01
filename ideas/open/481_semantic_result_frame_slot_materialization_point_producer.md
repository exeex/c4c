# Semantic Result Frame-Slot Materialization Point Producer

Status: Open
Type: Prepared explicit semantic result frame-slot materialization-point producer idea
Parent: `ideas/closed/480_semantic_instruction_result_frame_slot_write_event_producer.md`
Source Evidence: `build/agent_state/480_step4_semantic_write_event_producer_residual/`
Owning Layer: Prepared semantic result frame-slot materialization-point production

## Lifecycle Note

Parked by `ideas/open/482_semantic_frame_slot_materialization_probe_decomposition.md`.
The 475 -> 481 route repeatedly moved the first missing producer lower inside
the same `%t23` representative failure family without reducing the owned
capability family. Idea 481 remains valid source intent, but implementation
should resume only after focused decomposition probes identify a smaller
generic seam.

## Goal

Produce explicit prepared materialization-point facts proving that a semantic
instruction result is written or materialized into a specific frame slot.

## Why This Exists

Idea 480 could not produce a sound write/materialization event for
`%t23 = bir.ne i32 %t22, 0` into slot `#21` because current prepared/prealloc
evidence has semantic identity and final destination facts, but no explicit
point where `%t23` is known to be materialized into that frame slot. The
`%t22 -> %t23` move remains `authority=none`, has the wrong source, and is
storage movement rather than semantic compare-result materialization.

## In Scope

- Audit where semantic instruction results become explicit frame-slot
  materialization points in prepared/prealloc data.
- Define a durable materialization-point record with function, stable event
  site, semantic producer identity, result value id/name/type, and opcode.
- Record event source/result matching the semantic result.
- Record destination slot/object/offset/size/alignment.
- Record authority proving the semantic result was materialized into the
  destination frame slot.
- Preserve fail-closed statuses for missing materialization point,
  storage-only movement, source/result mismatch, destination mismatch,
  unsupported authority, and protected boundaries.

## Out Of Scope

- Consuming materialization-point records through ideas 478/479/480.
- Path/dominance coverage, same-slot write exclusion, and effect non-clobber
  interval proof.
- Populating semantic interval, `PreparedFrameSlotSourceFact`, or
  `PreparedBranchStackLoadAuthority` rows.
- RV64 branch-load emission or target lowering.
- Treating storage-only moves such as `%t22 -> %t23` as semantic
  materialization when authority is absent.
- Pointer-value/provenance repair, select-result stack-destination repair, or
  unsupported-terminator branch-site relationship repair.
- Inferring materialization points from raw BIR adjacency, branch conditions,
  final stack homes, storage, offsets, object ids, value names, function names,
  testcase names, or dump order.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.

## Acceptance Criteria

- Prepared materialization-point records are produced only from explicit
  producer evidence, not from raw instruction shape, final homes, storage, or
  names.
- Positive coverage demonstrates at least one scalar instruction-result
  materialization point with function/site, semantic result identity,
  destination slot/object/layout, and explicit authority.
- Negative coverage rejects raw instruction or branch-condition-only evidence,
  final home/storage/object-only evidence, `authority=none` storage movement,
  source/result mismatches, destination mismatches, unsupported authority,
  protected boundaries, and raw-shape fixtures.
- Fresh residual disposition states whether idea 480-style write-event
  production may resume or whether another materialization-point owner remains
  first.

## Reviewer Reject Signals

- Reject RV64 lowering changes presented as materialization-point producer
  work.
- Reject directly setting downstream write-event, event-authority, interval,
  source-fact, or branch-stack-load availability from this idea.
- Reject treating `%t22 -> %t23` or other storage-only moves as semantic
  instruction-result materialization when authority is absent.
- Reject inferring materialization points from raw BIR shape, branch
  conditions, final stack homes, storage, offsets, object ids, value names,
  function names, testcase names, or dump order.
- Reject folding path/no-clobber interval proof, pointer/provenance,
  select-result stack-destination, or unsupported-terminator repair into this
  materialization-point producer.
- Reject expectation rewrites, unsupported downgrades, allowlists, baseline
  churn, or classification-only changes claimed as capability progress.
