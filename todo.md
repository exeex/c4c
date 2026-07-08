Status: Active
Source Idea Path: ideas/open/596_pointer_rhs_branch_stack_source_policy_publication.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Record The 594 Handoff

# Current Packet

## Just Finished

Completed Step 4 lifecycle handoff inventory for idea 594. Idea 594 can be
reactivated for RV64 pointer `Rhs` consumer migration because selected shared
producer authority now exists for valid pointer `Rhs` branch stack-load uses.

The producer authority now available to 594 is selected
`PreparedValueFreshnessUseKind::BranchStackLoadSource` /
`PreparedValueFreshnessSourceKind::BranchStackSlot` authority for the same
prepared source value, exact branch block, exact terminator instruction index,
`PreparedValueFreshnessProofKind::BranchTerminatorOrdering`, and
`PreparedValueFreshnessSourceRank::BranchStackSlot`. Valid collected `Rhs`
rows now report `policy=LoadFromStackSlot`, `pointer_status=proven`, selected
source freshness, and `stack_slot_fresh_at_branch`.

Accepted proof covered direct planning and collected prepared-dump publication
for valid pointer `Rhs` authority. Rejected proof covered missing, ambiguous,
stale, wrong-value, wrong-use, future-point, and stack-home-only freshness
authority, plus the existing role-independent structural fail-closed cases for
unsupported homes, home/value mismatch, frame-slot mismatch, stack-object
mismatch, missing clobber safety, and unknown pointer status.

No producer-side blocker remains for 594's pointer `Rhs` consumer migration.
This executor packet did not edit or reactivate idea 594.

## Suggested Next

Supervisor can route lifecycle closure or reactivation decisions. The coherent
next implementation packet is in idea 594: migrate the RV64 pointer `Rhs`
consumer to require the selected shared producer authority named above.

## Watchouts

- 594 must still implement the RV64 consumer migration; this plan only repaired
  producer authority.
- No target-local consumer fallback was added. RV64 should consume the selected
  shared authority, not infer freshness from stack homes, frame slots,
  aggregate lanes, clobber facts, register facts, operand shape, or testcase
  shape.
- The selected `Rhs` route is exact-point authority only: same prepared source
  value, exact branch block, exact terminator instruction index, and
  `BranchTerminatorOrdering`/`BranchStackSlot` proof. Stale and future
  terminator-point candidates remain fail-closed.

## Proof

Proof command for this packet: none; handoff inventory only.

Previously accepted validation for this lifecycle handoff:

- Step 3 backend proof passed: `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1` reported `100% tests passed, 0 tests failed out of 346`.
- Regression guard passed.
- Hook-managed full-suite baseline after code commits was accepted at
  `3375/3375`.
