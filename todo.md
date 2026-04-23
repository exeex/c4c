Status: Active
Source Idea Path: ideas/open/87_out_of_ssa_contract_and_parallel_copy_authority_for_prealloc.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Contract Surface Audit
Plan Review Counter: 2 / 6
# Current Packet

## Just Finished

Step 1 audit found that `PreparedJoinTransfer` already publishes authoritative branch ownership,
incoming labels, and edge transfers, while `PreparedParallelCopyBundle` already publishes
cycle-breaking steps. The first bounded packet now makes the prepared dump summarize that
ownership directly with join-transfer counts/source indexes and parallel-copy resolution/linkage,
then proves those strings in backend printer coverage.

## Suggested Next

Step 1: finish the contract-surface audit by checking whether any out-of-SSA continuation
ownership or slot-backed carrier policy is still implicit in helpers/tests rather than visible from
the published control-flow records and dump output.

## Watchouts

- Keep phi elimination and parallel-copy authority in `out_of_ssa`; do not push semantics back into `legalize`.
- Treat grouped-register authority and prepared frame/stack/call work as separate ideas, not scope for this runbook.

## Proof

Pending: `cmake --build --preset default` and
`ctest --test-dir build -j --output-on-failure -R '^backend_'`.
