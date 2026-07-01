# Semantic Instruction Result Frame-Slot Materialization Facts

Status: Open
Type: Prepared semantic instruction-result materialization and interval fact producer idea
Parent: `ideas/closed/475_prepared_frame_slot_source_fact_population.md`
Source Evidence: `build/agent_state/475_step4_source_fact_population_residual/`
Owning Layer: Prepared semantic instruction-result frame-slot materialization/write and no-clobber interval facts

## Goal

Publish lower-level prepared facts that identify semantic instruction-result
frame-slot materialization/write events and prove the path/no-clobber interval
needed before source-fact population can resume.

## Why This Exists

Idea 475 proved the existing source-fact carrier can represent available facts,
but real rows still lack a producer fact saying `%t23 = ne i32 %t22, 0` was
materialized or written into slot `#21`. The nearby `%t22 -> %t23` stack move
has `authority=none` and cannot be used as semantic compare-result evidence.
Source-fact population, branch-stack-load authority, and RV64 consumption must
wait for explicit semantic materialization/write and interval facts.

## In Scope

- Audit semantic instruction-result identity for scalar instruction results,
  especially `%t23 = ne i32 %t22, 0`.
- Define prepared records for explicit frame-slot materialization/write events
  naming source value, destination frame slot/object/offset/size/alignment, and
  semantic producer kind.
- Prove path/dominance or edge-scope validity from materialization/write event
  to a consumer site.
- Prove same-slot write exclusion for the interval.
- Prove call/helper/inline-asm slot effect safety for the interval.
- Classify publication, move-bundle, and parallel-copy events as clobbering,
  non-clobbering, or unknown for the materialized slot.
- Preserve enough status detail for idea 475-style source-fact population to
  consume later.

## Out Of Scope

- Populating `PreparedFrameSlotSourceFact` rows directly unless this idea first
  produces the lower-level semantic source facts.
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

- Semantic instruction-result materialization/write records are produced only
  from explicit producer facts, not from raw shape or storage moves.
- Positive coverage demonstrates a scalar instruction result with exact
  semantic source identity, destination frame-slot/object identity, path or
  edge validity, same-slot write exclusion, effect safety, and
  publication/move/copy non-clobber classification.
- Negative coverage rejects missing semantic identity, storage-only moves,
  invalid path, same-slot writes, unknown call/helper/inline-asm effects,
  clobbering publications/moves/copies, select-result stack destinations,
  pointer/provenance gaps, unsupported terminators, and raw-shape fixtures.
- Fresh residual disposition states whether idea 475-style source-fact
  population can resume or whether another lower-level producer remains first.

## Reviewer Reject Signals

- Reject RV64 lowering changes presented as semantic materialization producer
  work.
- Reject directly setting downstream source-fact or branch-stack-load
  availability from this idea without explicit lower-level facts.
- Reject treating `%t22 -> %t23` or other storage-only moves as semantic
  instruction-result materialization when authority is absent.
- Reject inferring materialization/no-clobber facts from raw BIR shape, final
  stack homes, storage, offsets, object ids, value names, function names,
  testcase names, or dump order.
- Reject folding pointer-value/provenance, select-result stack-destination, or
  unsupported-terminator repair into this semantic materialization idea.
- Reject expectation rewrites, unsupported downgrades, allowlists, baseline
  churn, or classification-only changes claimed as capability progress.
