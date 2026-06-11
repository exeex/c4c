Status: Active
Source Idea Path: ideas/open/204_route5_edge_join_source_adapter.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Adapter Completeness And Route Quality

# Current Packet

## Just Finished

Step 4 proved the selected Route 5 current-block join-source adapter complete
for this runbook boundary. No focused adjacent tests were added because the
existing proof already covers the required route-quality cases.

`tests/backend/bir/backend_prepared_lookup_helper_test.cpp` proves available
indexed Route 5 current-block join facts, empty/incomplete index failure,
duplicate exact-record rejection, wrong-predecessor rejection, no-successor and
no-PHI failure, missing source value/destination mismatch, missing source
producer diagnosis, and memory/stack-source identity sanity. It also checks
that BIR semantic facts match prepared join-source facts without replacing
prepared move/storage authority.

`tests/backend/mir/backend_aarch64_current_block_join_routing_test.cpp` proves
the selected AArch64 boundary uses Route 5 only for semantic identity bitsets
when the adapter reports `Available`, falls back to prepared query behavior for
absent/no-source cases, preserves memory-source identity without treating it as
policy authority, and leaves prepared scheduling, homes, wrappers, branch
policy, final edge-copy records, and emitted output authority unchanged.

## Suggested Next

Supervisor can perform acceptance review, compare `test_before.log` and
`test_after.log` for the same backend CTest scope, then decide whether to
commit this completed slice or route lifecycle closure to the plan owner.

## Watchouts

- This Step 4 packet was proof-only; implementation and test files were left
  unchanged to avoid churn after coverage was found sufficient.
- Route 5 remains semantic identity only; prepared state remains authoritative
  for scheduling, storage, wrappers, branch policy, and output.
- Remaining non-goal: this runbook does not expand Route 5 beyond the selected
  current-block join-source reader boundary.
- Do not accept future progress through expected-output weakening, unsupported
  downgrades, helper renames, or testcase-shaped matching.

## Proof

Ran exactly:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: passed. Proof log path: `test_after.log`.
