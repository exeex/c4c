# Prepared Frame-Slot Source-Fact Population

Status: Open
Type: Prepared source-fact population producer idea
Parent: `ideas/closed/474_prepared_frame_slot_materialization_no_clobber_facts.md`
Source Evidence: `build/agent_state/474_step4_residual_disposition/`
Owning Layer: Prepared frame-slot materialization/write and no-clobber fact population

## Goal

Populate real prepared frame-slot source facts for scalar branch stack slots
using explicit materialization/write events, path/dominance validity, same-slot
write exclusion, call/helper/inline-asm effect safety, and
publication/move-bundle/parallel-copy non-clobber classification.

## Why This Exists

Idea 474 completed the independent carrier/status surface and proved the
planner is fail-closed. Real representative rows still remain unavailable
because no producer currently supplies materialization/write and no-clobber
facts for `%t23` slot `#21`. Downstream branch-stack-load authority and RV64
consumption must wait until real source facts exist.

## In Scope

- Audit real prepared evidence for scalar branch stack slots, especially
  `f.logic.end.14` condition `%t23`, slot `#21`.
- Produce exact frame-slot materialization/write facts naming source value,
  frame slot, and stack object.
- Prove path/dominance validity from the materialization/write event to the
  consumer site.
- Prove same-slot write exclusion after the materialization/write event.
- Prove call/helper/inline-asm effect safety for the slot.
- Classify publication, move-bundle, and parallel-copy events as non-clobbering
  only with explicit evidence.
- Populate existing prepared frame-slot source-fact records/statuses for real
  rows.

## Out Of Scope

- Defining another carrier/status surface unless Step 1 proves the existing
  idea 474 surface is structurally insufficient.
- RV64 branch-load emission or target lowering.
- Directly marking `PreparedBranchStackLoadAuthority` rows available.
- Branch-stack-load policy/freshness/clobber consumption.
- Pointer-value/provenance repair for `%t1` or `%t7`.
- Select-result/block-entry stack-destination repair for `%t22/%t23`.
- Branch-site relationship acceptance for `unsupported_terminator` rows.
- Inferring source facts from stack homes/storage, offsets, object ids, raw BIR
  adjacency, value names, block labels, function names, testcase names, or dump
  order.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.

## Acceptance Criteria

- Real prepared source-fact rows are populated only from explicit
  materialization/write, path validity, same-slot exclusion, effect safety, and
  publication/move/copy non-clobber evidence.
- Positive coverage demonstrates at least one real scalar branch stack-slot row
  with available or more precise source-fact status.
- Negative coverage preserves fail-closed statuses for missing
  materialization, invalid path, same-slot writes, unknown effects, clobbering
  publications/moves/copies, raw-shape fixtures, select-result stack
  destinations, pointer/provenance gaps, and unsupported terminators.
- Fresh residual disposition states whether downstream certificate or
  branch-stack-load authority consumption may resume.

## Reviewer Reject Signals

- Reject RV64 lowering changes presented as source-fact population work.
- Reject directly setting downstream branch-stack-load availability from this
  idea.
- Reject inferring materialization/current-value/no-clobber facts from stack
  homes/storage, offsets, object ids, raw BIR shape, value names, block labels,
  function names, testcase names, or dump order.
- Reject folding pointer-value/provenance, select-result stack-destination, or
  unsupported-terminator relationship repair into this population idea.
- Reject expectation rewrites, unsupported downgrades, allowlists, baseline
  churn, or classification-only changes claimed as capability progress.
