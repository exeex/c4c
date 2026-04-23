Status: Active
Source Idea Path: ideas/open/87_out_of_ssa_contract_and_parallel_copy_authority_for_prealloc.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Proof And Observation Tightening
Plan Review Counter: 2 / 6
# Current Packet

## Just Finished

Step 3 now proves the remaining loop-countdown handoff boundary: x86 loop-countdown coverage strips
`parallel_copy_bundles` while preserving `join_transfers` and confirms the prepared-module consumer
rejects missing authoritative parallel-copy publication instead of accepting phi-edge obligations
through a join-only fallback.

## Suggested Next

Step 3 follow-up: review whether any other prepared handoff family still lacks an isolated
`parallel_copy_bundles`-removed/`join_transfers`-preserved rejection proof, or hand the runbook back
for plan review if this was the last unresolved Step 3 fallback probe.

## Watchouts

- Authoritative branch-owned continuation consumers must fail closed when
  `continuation_true_label`/`continuation_false_label` publication is absent; rebuilding them from
  join or rhs CFG shape is now a contract regression.
- Loop-countdown now proves `join_transfers` alone are not enough to keep phi-edge obligations
  alive after `parallel_copy_bundles` are stripped; treat any future success in that shape as a
  contract regression.
- Keep phi elimination and parallel-copy authority in `out_of_ssa`; do not push semantics back into
  `legalize`, and keep grouped-register or frame/stack/call work out of this runbook.

## Proof

Passed: `bash -lc 'set -o pipefail; cmake --build --preset default 2>&1 | tee test_after.log && ctest --test-dir build -j --output-on-failure -R "^backend_" 2>&1 | tee -a test_after.log'`.
Proof log: `test_after.log`.
