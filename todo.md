Status: Active
Source Idea Path: ideas/open/87_out_of_ssa_contract_and_parallel_copy_authority_for_prealloc.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Join And Parallel-Copy Authority Completion
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Step 2 now finishes the authoritative continuation-label handoff for branch-owned joins: the shared
compare-join helper and prepared-control-flow dump no longer recompute continuation targets for
authoritative branch-owned joins once `out_of_ssa` has published `continuation_true_label` and
`continuation_false_label`. Coverage now hardens that contract across helper, printer, and x86
handoff surfaces by removing join/rhs branch metadata from the authoritative short-circuit fixtures
and proving the published labels still drive the result.

## Suggested Next

Step 3: tighten proof-and-observation coverage around the published join/parallel-copy authority,
especially any remaining non-authoritative compatibility fallbacks that still recover continuation
or edge-copy meaning from raw CFG shape.

## Watchouts

- Authoritative branch-owned continuation consumers should treat missing published continuation
  labels as a contract break, not as permission to rediscover labels from join/rhs block shape.
- The helper/printer tests now strip non-entry branch metadata from authoritative short-circuit
  fixtures; any future regression back to CFG recomputation should fail immediately.
- Keep phi elimination and parallel-copy authority in `out_of_ssa`; do not push semantics back into
  `legalize`, and keep grouped-register or frame/stack/call work out of this runbook.

## Proof

Passed: `bash -lc 'set -o pipefail; cmake --build --preset default 2>&1 | tee test_after.log && ctest --test-dir build -j --output-on-failure -R "^backend_" 2>&1 | tee -a test_after.log'`.
Proof log: `test_after.log`.
