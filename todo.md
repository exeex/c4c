Status: Active
Source Idea Path: ideas/open/87_out_of_ssa_contract_and_parallel_copy_authority_for_prealloc.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Cleanup And Consumer Confirmation
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Step 4 audited the remaining pre-out-of-SSA-looking paths in `legalize`/`out_of_ssa`/`regalloc`,
kept the explicit slot-backed `EdgeStoreSlot` carrier because it is still live published authority
rather than dead fallback, and added a new regalloc proof that select-materialized joins consume
published out-of-SSA join metadata through `phi_join_*` move resolution instead of generic
`consumer_*` reconstruction.

## Suggested Next

Step 4 follow-up: decide whether this runbook is complete enough to close, or hand it back for plan
review if a final supervisor/reviewer pass still wants broader consumer coverage beyond regalloc.

## Watchouts

- The surviving slot-backed phi path in `out_of_ssa` is still an intentional carrier contract
  (`EdgeStoreSlot`), not a removable legalize leftover; deleting it would widen this packet into a
  semantic change instead of cleanup.
- `regalloc` must keep treating select-materialized joins as published out-of-SSA authority and must
  not regress into `consumer_*` move synthesis for those results.
- No Step 4 code cleanup in `legalize.cpp` was justified by this audit; the proof landed as consumer
  confirmation rather than speculative deletion.

## Proof

Passed: `bash -lc 'set -o pipefail; cmake --build --preset default 2>&1 | tee test_after.log && ctest --test-dir build -j --output-on-failure -R "^backend_" 2>&1 | tee -a test_after.log'`.
Proof log: `test_after.log`.
