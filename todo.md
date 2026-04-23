Status: Active
Source Idea Path: ideas/open/87_out_of_ssa_contract_and_parallel_copy_authority_for_prealloc.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Proof And Observation Tightening
Plan Review Counter: 4 / 6
# Current Packet

## Just Finished

Step 3 now adds the missing short-circuit bundle-strip proof on top of the earlier compare-join and
loop-countdown coverage: the x86 short-circuit handoff suite strips authoritative
`parallel_copy_bundles` while preserving published `join_transfers` and confirms the prepared-module
consumer fails closed instead of accepting phi-edge obligations through a join-only fallback.

## Suggested Next

Step 3 follow-up: review whether any remaining prepared handoff family still lacks an isolated
`parallel_copy_bundles`-removed/`join_transfers`-preserved rejection proof, or hand the runbook
back for plan review if short-circuit was the last unresolved Step 3 fallback probe.

## Watchouts

- Authoritative branch-owned continuation consumers must fail closed when
  `continuation_true_label`/`continuation_false_label` publication is absent; rebuilding them from
  join or rhs CFG shape is now a contract regression.
- Compare-join, loop-countdown, and short-circuit now all prove `join_transfers` alone are not
  enough to keep phi-edge obligations alive after `parallel_copy_bundles` are stripped; treat any
  future success in any of those shapes as a contract regression.
- Keep phi elimination and parallel-copy authority in `out_of_ssa`; do not push semantics back into
  `legalize`, and keep grouped-register or frame/stack/call work out of this runbook.

## Proof

Passed: `bash -lc 'set -o pipefail; cmake --build --preset default 2>&1 | tee test_after.log && ctest --test-dir build -j --output-on-failure -R "^backend_" 2>&1 | tee -a test_after.log'`.
Proof log: `test_after.log`.
