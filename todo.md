Status: Active
Source Idea Path: ideas/open/590_branch_stack_load_freshness_contract.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Focused Proof

# Current Packet

## Just Finished

Completed Step 4, "Add Focused Proof", from `plan.md`.

Strengthened focused proof and prepared dump visibility for the migrated scalar
`Condition` branch stack-load route. The helper test now asserts the accepted
authority selected exactly one `BranchStackLoadSource`/`BranchStackSlot`
candidate with `BranchTerminatorOrdering` proof, `BranchStackSlot` rank, the
condition value/home, and the branch terminator block/instruction point. The
prepared branch-stack-load printer row now emits source-freshness status,
candidate count, selected authority, use/proof/rank, value, and reference
block/inst fields; the dump assertion requires those fields on the accepted
condition row and keeps the `Lhs` stack-home-only inventory row visibly
`policy=none` with no source-freshness candidate.

Missing/no-candidate, invalid, ambiguous, wrong-value, wrong-use, wrong-home,
stale terminator point, future terminator point, missing clobber safety, and
stack-home-only rejection coverage remained in the focused helper test. No
target emission files, expectation files, unsupported markers, allowlists, or
runtime-output tests were changed.

## Suggested Next

Execute Step 5 from `plan.md`: prepare closure inventory for the source idea by
summarizing audited consumers, the branch-point freshness ownership rule,
remaining unwired consumers, and whether typed/aggregate stack-source producer
facts should become a follow-up idea or stay deferred.

## Watchouts

- Step 5 should keep typed/aggregate stack-source producer facts and target
  emission migration as closure inventory or follow-up scope; this slice only
  proved the representative scalar `Condition` route.
- Pointer `Lhs`/`Rhs` collector rows remain inventory-only with `policy=none`,
  so they should not be described as migrated freshness-authorized routes.

## Proof

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`
passed. Proof log: `test_after.log` (`346` backend tests passed).
