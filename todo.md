Status: Active
Source Idea Path: ideas/open/593_rv64_branch_stack_source_freshness_consumption.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Wire The Selected RV64 Consumer To Shared Freshness

# Current Packet

## Just Finished

Completed Step 2 for `plan.md`: RV64 object emission now carries shared
`PreparedBranchStackLoadAuthorityRecords` through `PreparedFunctionLookups` and
requires an available selected pointer `Lhs`
`PreparedBranchStackLoadRole::Lhs` authority before
`fragment_for_prepared_fused_pointer_branch` may load a stack-slot `Lhs` for a
fused pointer branch.

The selected RV64 `Lhs` stack-source gate checks the same prepared value
id/name, `PreparedValueFreshnessUseKind::BranchStackLoadSource`,
`PreparedValueFreshnessSourceKind::BranchStackSlot`,
`PreparedValueFreshnessProofKind::BranchTerminatorOrdering`,
`PreparedValueFreshnessSourceRank::BranchStackSlot`, the same prepared stack
home, and the exact branch block plus terminator instruction point. Non-stack
`Lhs` operands stay on their existing path; pointer `Rhs`, scalar condition
register branches, string assembly emission, aggregate-adjacent branch
consumers, AArch64, and x86 remain out of scope.

## Suggested Next

Execute Step 3 narrowly for the same RV64 fused pointer `Lhs` path: add or
preserve visible diagnostics/status for missing selected source freshness and
keep that failure distinct from missing stack home, unsupported operand shape,
layout mismatch, and clobber-safety failures.

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
- `plan_prepared_fused_pointer_branch_publication` still models older
  GPR-compatible operand publication separately from the selected branch
  stack-source freshness gate; keep Step 3 diagnostics careful so missing
  freshness does not get hidden behind generic operand publication failures.

## Proof

Ran the supervisor-selected proof command:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`.

Result: passed, `100% tests passed, 0 tests failed out of 346`. Canonical proof
log: `test_after.log`.
