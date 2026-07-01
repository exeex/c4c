# Prepared Frame-Slot Source-Fact Population

Status: Closed
Type: Prepared source-fact population producer idea
Parent: `ideas/closed/474_prepared_frame_slot_materialization_no_clobber_facts.md`
Source Evidence: `build/agent_state/474_step4_residual_disposition/`
Owning Layer: Prepared frame-slot materialization/write and no-clobber fact population

## Goal

Populate real prepared frame-slot source facts for scalar branch stack slots
using explicit materialization/write events, path/dominance validity, same-slot
write exclusion, call/helper/inline-asm effect safety, and
publication/move-bundle/parallel-copy non-clobber classification.

## Completion Notes

Closed after Step 4 residual disposition as a routed/blocked population
attempt. Idea 475 verified that the carrier from idea 474 can represent the
needed source facts, but current prepared evidence cannot populate real records
for the representative branch condition.

The completed findings are:

- `f.logic.end.14` condition `%t23`, slot `#21`, has known branch condition,
  target home, frame slot, stack object, offset, size, and alignment.
- No current prepared record says `%t23 = ne i32 %t22, 0` was materialized or
  written into slot `#21`.
- No current prepared record proves path/dominance from such a materialization
  event to `logic.end.14`.
- Same-slot write exclusion, call/helper/inline-asm slot effect safety, and
  publication/move-bundle/parallel-copy non-clobber facts are absent.
- The nearby `%t22 -> %t23` stack move has `authority=none`, source value
  `#16`, destination value `#17`, and cannot be used as semantic compare-result
  materialization evidence.

Follow-up source idea:
`ideas/open/476_semantic_instruction_result_frame_slot_materialization_facts.md`.

Source-fact population, downstream branch-stack-load authority, and RV64
branch-load consumption remain blocked until lower-level semantic
materialization/write and interval no-clobber facts exist.

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
