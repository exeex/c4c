Status: Active
Source Idea Path: ideas/open/593_rv64_branch_stack_source_freshness_consumption.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Preserve Fail-Closed Diagnostics

# Current Packet

## Just Finished

Completed Step 3 for `plan.md`: RV64 object emission now preserves visible
status for the selected fused pointer `Lhs` stack-load freshness gate. The Step
2 boolean gate is backed by a local status carrier that records whether
freshness was required, whether selected authority was available, the shared
`PreparedBranchStackLoadAuthorityStatus`, the
`PreparedValueFreshnessQueryStatus`, and the candidate count.

When a stack-slot `Lhs` fused pointer branch cannot emit, RV64 terminator
diagnostics now distinguish source-freshness failures with
`unsupported_branch_stack_load_source_freshness` and report
`authority_status`, `source_freshness_status`, and
`source_freshness_candidates`. Other unavailable branch-stack-load authority
failures use `unsupported_branch_stack_load_authority`, so missing source
freshness stays distinct from stack/layout/clobber/shape authority failures.
Non-stack `Lhs` operands stay on their existing path; pointer `Rhs`, scalar
condition register branches, string assembly emission, aggregate-adjacent
branch consumers, AArch64, and x86 remain out of scope.

## Suggested Next

Execute Step 4 narrowly for the same RV64 fused pointer `Lhs` path: add focused
proof that accepted emission consumes selected shared `BranchStackLoadSource`
authority and that missing, stale, wrong-value, wrong-use, ambiguous,
future-point, and stack-home-only authority fail closed with the visible status
path added in Step 3.

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
- The Step 3 diagnostic path intentionally preserves non-source authority
  statuses instead of relabeling them as source freshness; Step 4 should assert
  the exact freshness cases, not weaken layout or clobber failures.
- Producer-side blocker: pointer `Rhs` is not available for the first RV64
  migration because the current producer/collector keeps it inventory-only
  (`policy=none`, `status=missing_policy`). Do not add an RV64 fallback for
  `Rhs`.
- `plan_prepared_fused_pointer_branch_publication` still models older
  GPR-compatible operand publication separately from the selected branch
  stack-source freshness gate; Step 4 proof should make sure missing freshness
  is not hidden behind generic operand publication failures.

## Proof

Ran the supervisor-selected proof command:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`.

Result: passed, `100% tests passed, 0 tests failed out of 346`. Canonical proof
log: `test_after.log`.
