Status: Active
Source Idea Path: ideas/open/596_pointer_rhs_branch_stack_source_policy_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Semantic Rhs Policy Selection

# Current Packet

## Just Finished

Completed Step 2 implementation for semantic `Rhs` branch stack-load producer
policy selection. `prepared_branch_stack_load_pointer_operand_is_proven` now
accepts the same prepared source value for pointer fused-compare `Lhs` or
`Rhs` operands, and the collected policy/status/clobber-safety helpers pass
`LoadFromStackSlot`, `pointer_status=proven`, and branch-point clobber safety
into `plan_prepared_branch_stack_load_authority` for valid `Rhs` rows.

Updated the focused prepared-side BIR contract so a valid collected
`role=rhs` row now reports selected `BranchStackLoadSource` /
`BranchStackSlot` authority with `BranchTerminatorOrdering` proof and
`BranchStackSlot` rank, while explicit wrong-use and stack-home-only `Rhs`
freshness inputs remain fail-closed.

## Suggested Next

Execute Step 3 only if the supervisor wants broader prepared-side proof beyond
the updated `backend_prepare_stack_layout` contract; otherwise move to Step 4
handoff notes for reactivating idea 594 after selected `Rhs` producer authority.

## Watchouts

- No target-local consumer fallback was added; RV64/AArch64/x86/string
  assembly consumers still need to consume selected shared producer authority.
- The selected `Rhs` route still depends on exact branch block and terminator
  instruction freshness selection in `plan_prepared_branch_stack_load_authority`.
  Existing stale, wrong-value, wrong-use, future-point, ambiguous, missing, and
  stack-home-only authority checks remain in the focused contract.
- `clang-format` is not installed in this container, so formatting was kept
  manual.

## Proof

Ran delegated proof command exactly:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: passed. `test_after.log` reports `100% tests passed, 0 tests failed
out of 346`.
