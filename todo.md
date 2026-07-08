Status: Active
Source Idea Path: ideas/open/592_typed_aggregate_branch_stack_source_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate One Blocked Branch Stack-Load Consumer

# Current Packet

## Just Finished

Completed `plan.md` Step 3 by migrating only the pointer
`PreparedBranchStackLoadRole::Lhs` collector route from inventory-only
`policy=none` to `LoadFromStackSlot` when the shared branch stack-slot
freshness candidate is selected.

The collector now proves the narrow pointer `Lhs` contract from the fused
pointer compare itself, requires the branch stack-slot freshness candidate
published at the exact branch terminator point, and requires a conservative
clobber-safety proof: no intervening instructions before the branch terminator
for the migrated `Lhs` route. Without those facts, the existing planner fails
closed as `missing_source_freshness_authority`,
`missing_stack_clobber_safety`, or `pointer_status_unknown`.

Focused coverage now expects the collected pointer `Lhs` row to be
`status=available`, `policy=load_from_stack_slot`,
`pointer_status=proven`, and `source_freshness_status=selected`. The same
fixture adds a stack-backed pointer `Rhs` row and verifies it still remains
`policy=none`, `status=missing_policy`, and inventory-only despite having a
freshness candidate.

## Suggested Next

Proceed to `plan.md` Step 4 by adding focused accepted and fail-closed proof
around the migrated `Lhs` route, especially explicit collector fixtures for
missing, ambiguous, stale, wrong-value, wrong-use, future-point, and
stack-home-only authority.

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
