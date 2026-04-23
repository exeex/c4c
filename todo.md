Status: Active
Source Idea Path: ideas/open/87_out_of_ssa_contract_and_parallel_copy_authority_for_prealloc.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Join And Parallel-Copy Authority Completion
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Step 2 now publishes authoritative continuation targets directly on branch-owned out-of-SSA join
transfers via `continuation_true_label` and `continuation_false_label`, fills those labels for both
generic branch-owned joins and short-circuit joins, and switches helper/printer paths to consume
published labels before recomputing join-block shape. Coverage now locks the contract in two ways:
`backend_prepare_authoritative_join_ownership_test` verifies published continuation labels survive
after the join block loses the shape needed for recomputation, and `backend_prepared_printer_test`
verifies the dump still prints `continuation_targets=(...)` when both join-block shape and rhs
continuation recomputation are intentionally disabled.

## Suggested Next

Step 3: move the remaining branch-target consumers that still rediscover compare-join or
short-circuit routing onto the published join-transfer continuation labels and parallel-copy
authority fields, then trim any now-redundant shape-recovery fallback paths that no longer serve as
compatibility scaffolding.

## Watchouts

- Keep short-circuit continuation publication authoritative on the join transfer itself; helper
  fallbacks are now compatibility paths, not the primary contract.
- The printer test now deliberately sabotages both join-block and rhs-branch recomputation before
  printing, so future refactors that remove published-label preference will fail fast.
- Keep phi elimination and parallel-copy authority in `out_of_ssa`; do not push semantics back into
  `legalize`, and keep grouped-register or frame/stack/call work out of this runbook.

## Proof

Passed: `bash -lc 'set -o pipefail; cmake --build --preset default 2>&1 | tee test_after.log && ctest --test-dir build -j --output-on-failure -R "^backend_" 2>&1 | tee -a test_after.log'`.
Proof log: `test_after.log`.
