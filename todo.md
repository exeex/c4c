Status: Active
Source Idea Path: ideas/open/594_rv64_branch_stack_source_consumption_followup_from_593.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm Handoff And Locate Rhs Consumer

# Current Packet

## Just Finished

Completed `plan.md` Step 1 audit: confirmed the 593/596 handoff and located
the RV64 pointer `Rhs` consumer route before code edits.

593 closure-note gap: `ideas/closed/593_rv64_branch_stack_source_freshness_consumption.md`
lines 136-173 say the migrated route was the prepared RV64 object-emission
fused pointer conditional branch path for a stack-slot `Lhs`; pointer `Rhs`
remained unwired because the producer/collector recorded it as inventory-only
with `policy=none` / `status=missing_policy`; and 594 must block on producer
repair instead of adding an RV64 fallback if that policy is still missing.

596 closure-note authority now available:
`ideas/closed/596_pointer_rhs_branch_stack_source_policy_publication.md`
lines 77-101 say valid pointer `Rhs` branch stack-load uses now publish
selected `PreparedValueFreshnessUseKind::BranchStackLoadSource` /
`PreparedValueFreshnessSourceKind::BranchStackSlot` authority for the same
prepared source value, exact branch block, exact terminator instruction index,
`PreparedValueFreshnessProofKind::BranchTerminatorOrdering`, and
`PreparedValueFreshnessSourceRank::BranchStackSlot`; valid collected `Rhs`
rows report `policy=LoadFromStackSlot`, `pointer_status=proven`, selected
source freshness, and `stack_slot_fresh_at_branch`.

Migrated `Lhs` RV64 consumer path:
`src/backend/mir/riscv/codegen/object_emission.cpp` has
`selected_lhs_branch_stack_load_source_freshness_status()` at lines 9435-9539.
It scans `lookups->branch_stack_load_authorities.records`, requires
`PreparedBranchStackLoadRole::Lhs`, the same `block_label_id`, value id/name,
branch block index, and terminator instruction index, then accepts only
selected `BranchStackLoadSource` / `BranchStackSlot` freshness with
`BranchTerminatorOrdering`, `BranchStackSlot` rank, the same home pointer, and
the same branch point. `fragment_for_prepared_fused_pointer_branch()` lines
9584-9663 calls the Lhs freshness gate before planning the fused pointer
branch and before `append_rv64_move_value_to_register()` loads operands into
RV64 scratch registers x28/x29.

Adjacent pointer `Rhs` consumer path: the same
`fragment_for_prepared_fused_pointer_branch()` obtains `rhs_home` at lines
9602-9605 and later moves `normalized->rhs` into x29 at lines 9651-9657, but
there is no `Rhs` equivalent of the Lhs freshness gate. The unsupported
terminator diagnostic path is also Lhs-only at lines 9809-9862. This is the
exact RV64 pointer `Rhs` consumer to migrate.

Shared authority query to use: mirror the Lhs query against
`lookups->branch_stack_load_authorities.records`, but require
`PreparedBranchStackLoadRole::Rhs`, `record.block_label == block_label_id`,
`authority.value_id/value_name == rhs_home->value_id/value_name`,
`authority.branch_block_index == block_index`, and
`authority.branch_terminator_instruction_index == terminator_instruction_index`.
The selected freshness must pass the same contract used by the shared lookup in
`src/backend/prealloc/prepared_lookups.cpp` lines 97-114 and the producer
selector in `src/backend/prealloc/publication_plans.cpp` lines 2688-2739:
`BranchStackLoadSource`, `BranchStackSlot`, `BranchTerminatorOrdering`,
`BranchStackSlot` rank, same home, same block index, and same instruction
index.

Producer authority confirmation: producer-side code now includes both pointer
roles in `prepared_collected_branch_stack_load_policy()` and clobber safety
(`src/backend/prealloc/publication_plans.cpp` lines 2617-2686), publishes
branch stack-slot freshness at lines 2765-2799, and focused tests confirm
valid `Rhs` publication/selection in
`tests/backend/bir/backend_prepare_stack_layout_test.cpp` lines 6771-6797,
7292-7328, and 7793-7827. No producer-side blocker was found.

## Suggested Next

Execute `plan.md` Step 2 by adding a narrow `Rhs` selected-freshness gate in
`src/backend/mir/riscv/codegen/object_emission.cpp` beside the existing Lhs
gate, then require it in `fragment_for_prepared_fused_pointer_branch()` before
RV64 emits from a stack-homed pointer `Rhs`.

## Watchouts

- Do not add an RV64 fallback if selected shared producer authority is missing.
- Do not infer freshness from stack homes, frame slots, aggregate lanes,
  clobber facts, register facts, operand shape, or testcase shape.
- Keep pointer `Rhs` consumer migration separate from 591 Prepared MIR view
  research and 595 umbrella triage.
- Keep layout, stack home, and clobber checks as support facts only. Adjacent
  fail-closed cases to preserve/prove in Step 2/3: missing selected freshness,
  ambiguous selected freshness, invalid/stale/wrong-value/wrong-use/future-point
  freshness, stack-home-only freshness, missing frame slot/layout, stack object
  mismatch, home/value mismatch, missing clobber safety, unsupported operand
  shape, and unknown pointer status.
- Existing focused RV64 test surface for the migrated Lhs route is
  `tests/backend/mir/backend_riscv_object_emission_test.cpp` lines 739-774 and
  12631-13020; Step 3 should add the analogous `Rhs` object/proof coverage
  without weakening expectations.

## Proof

Audit-only packet. Read-only inspection used `sed`, `nl`, `rg`, and
`git status --short`; no build/tests were run and no root-level proof log was
created or modified.
