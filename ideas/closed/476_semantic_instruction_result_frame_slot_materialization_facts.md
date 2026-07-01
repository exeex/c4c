# Semantic Instruction Result Frame-Slot Materialization Facts

Status: Closed
Type: Prepared semantic instruction-result materialization and interval fact producer idea
Parent: `ideas/closed/475_prepared_frame_slot_source_fact_population.md`
Source Evidence: `build/agent_state/475_step4_source_fact_population_residual/`
Owning Layer: Prepared semantic instruction-result frame-slot materialization/write and no-clobber interval facts

## Goal

Publish lower-level prepared facts that identify semantic instruction-result
frame-slot materialization/write events and prove the path/no-clobber interval
needed before source-fact population can resume.

## Completion Notes

Closed after Step 4 residual disposition as complete for the independent
prepared semantic materialization / interval status surface.

Step 3 added:

- `PreparedSemanticMaterializationIntervalInputs`;
- `PreparedSemanticMaterializationInterval`;
- `PreparedSemanticMaterializationEventKind`;
- `PreparedSemanticMaterializationIntervalStatus`;
- `plan_prepared_semantic_materialization_interval`;
- `prepared_semantic_materialization_interval_available`.

The completed status surface accepts only explicit semantic result identity,
explicit materialization/write event, matching destination slot/object,
consumer-covering path proof, same-slot write exclusion, and non-clobber
classification for call/helper, publication, move-bundle, and parallel-copy
effects. It rejects missing semantic identity, missing write event,
storage-only source moves such as `%t22 -> %t23`, destination mismatches,
missing or non-covering path proof, same-slot writes, unknown/clobbering
effects, and protected boundary rows.

Representative residual:

- Real `930930-1` `%t23 = ne i32 %t22, 0` into slot `#21` still lacks
  producer-populated materialization/write event and path/no-clobber interval
  facts.
- `%t22 -> %t23` storage movement has `authority=none` and remains rejected as
  semantic compare-result materialization evidence.
- Idea 475-style source-fact population, branch-stack-load authority, and RV64
  branch-load consumption remain blocked.

Follow-up source idea:
`ideas/open/477_real_semantic_materialization_interval_fact_population.md`.

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
