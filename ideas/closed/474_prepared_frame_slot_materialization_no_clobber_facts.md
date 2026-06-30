# Prepared Frame-Slot Materialization No-Clobber Facts

Status: Closed
Type: Prepared source-fact carrier and producer idea
Parent: `ideas/closed/473_branch_site_stack_slot_materialization_no_clobber_source.md`
Source Evidence: `build/agent_state/473_step4_residual_disposition/`
Owning Layer: Prepared frame-slot materialization/write and no-clobber facts

## Goal

Publish a durable prepared source-fact surface for frame-slot
materialization/write events, path validity, same-slot write exclusion,
call/helper/inline-asm safety, and publication/move-bundle/parallel-copy
non-clobber classification.

## Completion Notes

Closed after Step 4 residual disposition as complete for the independent
prepared carrier/status surface.

Step 3 added:

- `PreparedFrameSlotSourceFactStatus`;
- `PreparedFrameSlotSourceFactMaterializationKind`;
- `PreparedFrameSlotSourceFactInputs`;
- `PreparedFrameSlotSourceFact`;
- `PreparedFrameSlotSourceFactRecord(s)`;
- `plan_prepared_frame_slot_source_fact`;
- `prepared_frame_slot_source_fact_available`;
- `collect_prepared_frame_slot_source_facts`.

The carrier/status surface is fail-closed. It returns `available` only when
explicit inputs provide target/value/home/frame-slot/object coherence, source
value identity, explicit materialization/write event, path coverage, same-slot
write exclusion, call/helper effect safety, and publication/move-bundle/
parallel-copy non-clobber classification.

The Step 3 review initially found boundary rows were blurred into generic
`missing_materialization_event`. The follow-up fix and Step 4 disposition
record that select-result stack-destination, pointer/provenance, and
unsupported-terminator rows now remain `unsupported_boundary`.

Representative residuals:

- Synthetic explicit source facts are covered by idea 474.
- `f.logic.end.14` scalar condition `%t23`, slot `#21`, still lacks real
  materialization/write and no-clobber evidence, so source-fact population is
  the next first owner.
- `%t22` remains a select-result/block-entry stack-destination owner.
- `%t1` / `%t7` remain pointer/provenance owners.
- `%t2` / `%t8` remain unsupported-terminator relationship owners.
- Downstream `PreparedBranchStackLoadAuthority` remains unavailable.

Follow-up source idea:
`ideas/open/475_prepared_frame_slot_source_fact_population.md`.

Downstream branch-stack-load authority and RV64 branch-load consumption remain
blocked until real source facts are available and later packets consume them.

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
