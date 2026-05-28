Status: Active
Source Idea Path: ideas/open/62_aarch64_shared_edge_dependency_authority.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Remove Or Fail-Close Null-Publication Recovery

# Current Packet

## Just Finished

Step 4 - Remove Or Fail-Close Null-Publication Recovery completed.

Retired the remaining predecessor-depth producer discovery from the AArch64
edge-copy route by removing `find_edge_named_producer(...)` and
`unique_branch_predecessor_context(...)` from
`src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp` and their header
exports. The edge publication entry points now fail closed when invoked without
a prepared publication, so null publication no longer cues AArch64-local
semantic producer recovery.

Prepared-publication dependency walking still preserves target-local scratch
choice, clobber-sensitive ordering, register spelling, va-list carrier
emission, and instruction sequencing. Non-edge select-chain discovery and
branch-fusion behavior were not changed.

## Suggested Next

Step 5 - Prove Edge Coverage And Target-Local Preservation: review the Step 4
diff for renamed fallback scans or overfit behavior, then decide whether to run
broader regression guard or close/refresh the runbook state.

## Watchouts

`prepared_edge_named_source_producer_context(...)` remains reachable only under
a non-null prepared publication while recursively materializing dependencies of
that publication. The null-publication entry path now fails before it can use
that helper. Review should verify that this retained dependency walk is not
treated as a replacement edge-source authority.

## Proof

`(cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Proof passed: 169/169 backend tests passed. Log path: `test_after.log`.
