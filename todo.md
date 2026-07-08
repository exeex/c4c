Status: Active
Source Idea Path: ideas/open/592_typed_aggregate_branch_stack_source_publication.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Accepted And Fail-Closed Cases

# Current Packet

## Just Finished

Completed `plan.md` Step 4 by adding focused proof for the migrated pointer
`PreparedBranchStackLoadRole::Lhs` branch stack-load route.

The branch stack-load contract test now requires the accepted `Lhs` route to
select an explicit producer-published `BranchStackSlot` freshness authority
for `BranchStackLoadSource` at the exact branch terminator point. The selected
authority must match the `Lhs` stack home, value id/name, branch block index,
terminator instruction index, `BranchTerminatorOrdering` proof, and
`BranchStackSlot` rank before the route becomes available.

The same focused test now covers fail-closed `Lhs` cases for missing,
ambiguous, stale, wrong-value, wrong-use, future-point, and stack-home-only
authority. Existing `Rhs` inventory-only behavior remains blocked/out of
scope, and no expectation, unsupported-marker, allowlist, target emission, or
source-idea changes were made.

## Suggested Next

Proceed to `plan.md` Step 5 with closure inventory: summarize the audited
producer rule, migrated `Lhs` consumer, covered fail-closed cases, and the
deliberately blocked `Rhs`, aggregate-adjacent, select/edge, target-emission,
RV64, AArch64, and x86 work.

## Watchouts

- Only pointer `Lhs` was migrated. `Rhs`, aggregate-adjacent consumers,
  select/edge consumers, and target emission remain out of scope and blocked.
- The `Lhs` clobber proof is intentionally conservative: it only accepts a
  branch block with no intervening instructions before the terminator. Wider
  clobber classification belongs in a later packet.
- The current frame-slot source-fact collector treats pointer values and
  select-materialization results as unsupported boundaries; do not reuse those
  unsupported structural rows as later authority.
- Aggregate-adjacent authority currently exposes concrete source facts but
  reports missing aggregate copy authority; treat those facts as inputs to a
  future producer rule, not as direct branch freshness.
- Do not accept a branch stack source because a stack home, frame slot,
  aggregate lane, or clobber-safety fact exists.
- Do not use target-local branch emission inference as freshness authority.
- Do not weaken expectations, unsupported markers, allowlists, or runtime
  contracts as proof of capability progress.
- Keep broader branch, select, edge-publication, RV64, AArch64, and x86
  migrations out of the first packet.

## Proof

Ran the delegated proof command:

```bash
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1
```

Result: passed, with `test_after.log` reporting 346 passed, 0 failed.
