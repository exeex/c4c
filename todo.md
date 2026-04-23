Status: Active
Source Idea Path: ideas/open/87_out_of_ssa_contract_and_parallel_copy_authority_for_prealloc.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Contract Surface Audit
Plan Review Counter: 3 / 6
# Current Packet

## Just Finished

Step 1 audit found that `PreparedJoinTransfer` already publishes authoritative branch ownership,
incoming labels, and edge transfers, while `PreparedParallelCopyBundle` already publishes
cycle-breaking steps. The first bounded packet now makes the prepared dump summarize that
ownership directly with join-transfer counts/source indexes and parallel-copy resolution/linkage,
then proves those strings in backend printer coverage.
This packet extends the dump so authoritative compare-join continuation targets are visible on the
`join_transfer` summary itself instead of staying implicit in helper-only reconstruction from extra
branch-condition rows. Proof is green with `cmake --build --preset default` and
`ctest --test-dir build -j --output-on-failure -R '^backend_'`.

## Suggested Next

Step 1: finish the contract-surface audit by checking whether slot-backed carrier policy still
depends on helper-only interpretation, or whether the current `carrier=edge_store_slot` plus
published `storage=` / edge-transfer records already make that ownership sufficiently explicit.

## Watchouts

- Keep phi elimination and parallel-copy authority in `out_of_ssa`; do not push semantics back into `legalize`.
- Treat grouped-register authority and prepared frame/stack/call work as separate ideas, not scope for this runbook.

## Proof

Passed: `cmake --build --preset default` and
`ctest --test-dir build -j --output-on-failure -R '^backend_'`.
