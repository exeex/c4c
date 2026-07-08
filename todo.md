Status: Active
Source Idea Path: ideas/open/593_rv64_branch_stack_source_freshness_consumption.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Producer Contract And RV64 Consumers

# Current Packet

## Just Finished

Completed Step 1 audit for `plan.md`: inspected the idea 592 producer
contract, shared branch stack-load freshness helpers, and RV64 branch emission
consumers.

Idea 592 now publishes selected `PreparedValueFreshnessSourceKind::BranchStackSlot`
facts for `PreparedValueFreshnessUseKind::BranchStackLoadSource` at the exact
branch block plus terminator instruction point, with
`PreparedValueFreshnessProofKind::BranchTerminatorOrdering` and
`PreparedValueFreshnessSourceRank::BranchStackSlot`. Shared prealloc proves
scalar condition stack loads and pointer `PreparedBranchStackLoadRole::Lhs`;
pointer `Rhs` remains inventory-only with `policy=none` /
`missing_policy`, despite having a candidate freshness row, and should not be
chosen for the first RV64 migration.

Selected Step 2 consumer path: RV64 object emission
`fragment_for_prepared_fused_pointer_branch` in
`src/backend/mir/riscv/codegen/object_emission.cpp`, specifically the pointer
`Lhs` stack-source load before emitting the fused pointer branch. Today that
path validates only `plan_prepared_fused_pointer_branch_publication` and then
loads operands through `append_rv64_move_value_to_register`, which can accept a
prepared stack home / frame-slot offset as enough evidence.

Shared freshness query for Step 2: require a selected
`PreparedValueFreshnessQuery` via
`prepare::find_prepared_value_freshness_authority`, matching the `Lhs`
prepared value id/name, `PreparedValueFreshnessUseKind::BranchStackLoadSource`,
the exact RV64 branch block index, the terminator instruction index
`block.insts.size()`, and candidates from the producer-published
`BranchStackSlot` authority. The selected authority must match
`BranchStackSlot`, `BranchTerminatorOrdering`, `BranchStackSlot` rank, and the
same prepared stack home.

## Suggested Next

Implement Step 2 narrowly for the selected RV64 object-emission pointer `Lhs`
path: before `fragment_for_prepared_fused_pointer_branch` loads the `Lhs`
operand from a stack slot, require the shared selected
`BranchStackLoadSource` / `BranchStackSlot` freshness authority for that exact
branch terminator point. Leave pointer `Rhs`, string assembly emission in
`prepared_scalar_emit.cpp`, scalar condition register branches, and
aggregate-adjacent branch consumers for later inventory/follow-up work.

## Watchouts

- Do not start 594 before 593 closes with a concrete closure-note handoff.
- Do not use RV64 target-local stack-home, frame-slot, aggregate-lane, clobber,
  register, or operand-shape evidence as freshness.
- If a producer fact promised by 592 is missing, record that as a blocker for
  the 592 family instead of manufacturing fallback freshness in RV64.
- Structural freshness proxy sites found:
  `object_emission.cpp::fragment_for_prepared_fused_pointer_branch` uses
  `plan_prepared_fused_pointer_branch_publication` plus generic operand
  loading; `object_emission.cpp::append_rv64_move_value_to_register` accepts
  `prepared_stack_slot_home_absolute_offset_for_value` and emits a stack load;
  `prepared_scalar_emit.cpp::emit_riscv_prepared_fused_compare_branch` uses
  `emit_move_to_register`; and `prepared_scalar_emit.cpp::emit_move_to_register`
  accepts register homes, pointer register homes, stack-slot homes, and
  pointer-base-plus-offset homes directly.
- Shared prealloc still uses frame-slot/object/clobber checks in
  `plan_prepared_branch_stack_load_authority`, but only after source freshness
  selection; RV64 Step 2 must keep those as layout/safety checks, not as
  fallback freshness.
- Producer-side blocker: pointer `Rhs` is not available for the first RV64
  migration because the current producer/collector keeps it inventory-only
  (`policy=none`, `status=missing_policy`). Do not add an RV64 fallback for
  `Rhs`.
- Step 2 may need to expose branch stack-load authority records to RV64 object
  emission, since `PreparedFunctionLookups` currently does not carry
  `PreparedBranchStackLoadAuthorityRecords`; use shared prealloc APIs rather
  than a target-local freshness reconstruction.

## Proof

Audit-only packet per supervisor instruction. No build/tests run and no
root-level proof logs created.
