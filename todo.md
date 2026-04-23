Status: Active
Source Idea Path: ideas/open/87_out_of_ssa_contract_and_parallel_copy_authority_for_prealloc.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Proof And Observation Tightening
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Step 3 now tightens proof around published out-of-SSA authority: the prepared printer stops
reconstructing compare-join continuation targets, helper coverage proves authoritative joins return
no continuation targets once publication is stripped, and x86 compare-join coverage proves removing
authoritative out-of-SSA join metadata reopens no raw CFG fallback.

## Suggested Next

Step 3 follow-up: probe whether any remaining parallel-copy-only consumer still accepts
`join_transfers` edge data after `parallel_copy_bundles` publication is removed, and if so close
that last fallback in an owned packet.

## Watchouts

- Authoritative branch-owned continuation consumers must fail closed when
  `continuation_true_label`/`continuation_false_label` publication is absent; rebuilding them from
  join or rhs CFG shape is now a contract regression.
- The compare-join x86 packet now proves missing authoritative out-of-SSA join metadata fails the
  prepared handoff, but this packet did not yet isolate a separate runtime consumer that can still
  recover edge-copy meaning from `join_transfers` after `parallel_copy_bundles` are stripped.
- Keep phi elimination and parallel-copy authority in `out_of_ssa`; do not push semantics back into
  `legalize`, and keep grouped-register or frame/stack/call work out of this runbook.

## Proof

Passed: `bash -lc 'set -o pipefail; cmake --build --preset default 2>&1 | tee test_after.log && ctest --test-dir build -j --output-on-failure -R "^backend_" 2>&1 | tee -a test_after.log'`.
Proof log: `test_after.log`.
