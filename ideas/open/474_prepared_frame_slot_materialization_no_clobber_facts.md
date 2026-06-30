# Prepared Frame-Slot Materialization No-Clobber Facts

Status: Open
Type: Prepared source-fact carrier and producer idea
Parent: `ideas/closed/473_branch_site_stack_slot_materialization_no_clobber_source.md`
Source Evidence: `build/agent_state/473_step4_residual_disposition/`
Owning Layer: Prepared frame-slot materialization/write and no-clobber facts

## Goal

Publish a durable prepared source-fact surface for frame-slot
materialization/write events, path validity, same-slot write exclusion,
call/helper/inline-asm safety, and publication/move-bundle/parallel-copy
non-clobber classification.

## Why This Exists

Idea 473 could not produce branch-site stack-slot source facts because the
prepared/prealloc model has no independent carrier for exact materialization or
no-clobber evidence. Downstream branch-stack-load authority can only resume
after this lower-level source-fact layer exists.

## In Scope

- Audit existing prepared facts for frame-slot writes, value homes, stack
  objects, calls/helpers/inline asm, publications, move bundles, and parallel
  copies near `%t23` slot `#21`.
- Define a durable record/status surface for frame-slot materialization/write
  events independent of branch-stack-load authority rows.
- Record exact source value plus frame-slot/object identity for materialization
  or write events.
- Define path/dominance validity from a materialization/write event to a
  consumer site.
- Define same-slot write exclusion for a frame slot/object.
- Define call/helper/inline-asm effect safety for a frame slot/object.
- Classify publication, move-bundle, and parallel-copy events as clobbering,
  non-clobbering, or unknown for the target slot.

## Out Of Scope

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

- A prepared record/status surface exists for frame-slot materialization/write
  events and slot no-clobber facts.
- Positive coverage demonstrates exact source value, frame-slot/object
  identity, path validity, same-slot write exclusion, effect safety, and
  publication/move/parallel-copy non-clobber classification for a scalar case.
- Negative coverage rejects missing materialization, invalid paths, same-slot
  writes, unknown call/helper/inline-asm effects, clobbering publications,
  clobbering move bundles, raw-shape fixtures, pointer/provenance gaps,
  select-result stack destinations, and unsupported terminator rows.
- Fresh residual disposition states whether branch-site stack-slot source-fact
  consumption may resume or whether another source-fact owner remains first.

## Reviewer Reject Signals

- Reject RV64 lowering changes presented as prepared source-fact producer work.
- Reject directly setting downstream branch-stack-load availability from this
  idea.
- Reject inferring materialization/current-value/no-clobber facts from stack
  homes/storage, offsets, object ids, raw BIR shape, value names, block labels,
  function names, testcase names, or dump order.
- Reject folding pointer-value/provenance, select-result stack-destination, or
  unsupported-terminator branch-site relationship repair into this source-fact
  carrier idea.
- Reject expectation rewrites, unsupported downgrades, allowlists, baseline
  churn, or classification-only changes claimed as capability progress.
