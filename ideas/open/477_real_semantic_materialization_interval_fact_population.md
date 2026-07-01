# Real Semantic Materialization Interval Fact Population

Status: Open
Type: Prepared semantic materialization/write and interval fact population idea
Parent: `ideas/closed/476_semantic_instruction_result_frame_slot_materialization_facts.md`
Source Evidence: `build/agent_state/476_step4_semantic_materialization_interval_residual/`
Owning Layer: Real prepared semantic materialization/write and no-clobber interval facts

## Goal

Populate real prepared semantic instruction-result frame-slot
materialization/write records and path/no-clobber interval facts for scalar
branch stack slots.

## Why This Exists

Idea 476 completed an independent status surface that accepts explicit
semantic result identity, write event, destination slot/object, path proof, and
non-clobber classifications. Real representative rows still lack producer
population for `%t23 = ne i32 %t22, 0` into slot `#21`, so source-fact
population, branch-stack-load authority, and RV64 consumption remain blocked.

## In Scope

- Audit real prepared evidence for semantic instruction results, especially
  `%t23 = ne i32 %t22, 0`.
- Populate semantic instruction-result materialization/write event records
  naming the semantic result, producer opcode/kind, destination frame
  slot/object/offset/size/alignment, and source/destination value identity.
- Populate path/dominance or edge-scope proof from materialization/write event
  to the consumer site.
- Populate same-slot write exclusion for the interval.
- Populate call/helper/inline-asm slot effect safety for the interval.
- Populate publication, move-bundle, and parallel-copy non-clobber
  classifications for the interval.
- Preserve status detail needed for later idea 475-style source-fact
  population to consume available records.

## Out Of Scope

- Defining another independent status surface unless Step 1 proves the idea
  476 surface is structurally insufficient.
- Directly populating `PreparedFrameSlotSourceFact` rows.
- Directly marking `PreparedBranchStackLoadAuthority` rows available.
- RV64 branch-load emission or target lowering.
- Reusing storage-only moves such as `%t22 -> %t23` as semantic
  materialization evidence when `authority=none`.
- Pointer-value/provenance repair for `%t1` or `%t7`.
- Select-result/block-entry stack-destination repair for `%t22/%t23`.
- Branch-site relationship acceptance for `unsupported_terminator` rows.
- Inferring materialization or no-clobber facts from raw BIR adjacency, final
  stack homes, storage, offsets, object ids, value names, function names,
  testcase shape, or dump order.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.

## Acceptance Criteria

- Real semantic materialization/write records are populated only from explicit
  producer evidence, not from raw shape, final stack homes, or storage-only
  moves.
- Positive coverage demonstrates a real scalar instruction result with exact
  semantic identity, frame-slot/object destination, path or edge validity,
  same-slot write exclusion, effect safety, and publication/move/copy
  non-clobber classification.
- Negative coverage rejects missing semantic identity, missing write event,
  storage-only moves with no authority, invalid paths, same-slot writes,
  unknown or clobbering effects, select-result stack destinations,
  pointer/provenance gaps, unsupported terminators, and raw-shape fixtures.
- Fresh residual disposition states whether idea 475-style source-fact
  population may resume or whether another real producer remains first.

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
