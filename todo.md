Status: Active
Source Idea Path: ideas/open/596_pointer_rhs_branch_stack_source_policy_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Accepted And Rejected Producer Routes

# Current Packet

## Just Finished

Completed Step 3 focused prepared-side proof for pointer `Rhs` branch
stack-load authority. The existing contract already proved valid `Rhs`
publication through both direct planning and the collected prepared dump:
selected `BranchStackLoadSource` / `BranchStackSlot` authority,
`BranchTerminatorOrdering` proof, `BranchStackSlot` rank, and
`pointer_status=proven`.

Strengthened `backend_prepare_stack_layout` so `Rhs` now explicitly rejects
missing, ambiguous, stale, wrong-value, wrong-use, future-point, and
stack-home-only freshness authority. The shared role-independent structural
checks continue to cover unsupported homes, home/value mismatch, frame-slot
mismatch, stack-object mismatch, missing clobber safety, and unknown pointer
status.

## Suggested Next

Execute Step 4 handoff notes: selected `Rhs` producer authority is ready for
idea 594 consumer migration; do not edit or reactivate idea 594 from this plan.

## Watchouts

- No target-local consumer fallback was added; RV64/AArch64/x86/string
  assembly consumers still need to consume selected shared producer authority.
- The selected `Rhs` route depends on exact branch block and terminator
  instruction freshness selection in `plan_prepared_branch_stack_load_authority`;
  stale and future terminator-point candidates remain fail-closed.
- `clang-format` is not installed in this container, so formatting was kept
  manual.

## Proof

Ran delegated proof command exactly:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: passed. `test_after.log` reports `100% tests passed, 0 tests failed
out of 346`.
