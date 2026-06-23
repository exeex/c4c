Status: Active
Source Idea Path: ideas/open/310_rv64_c_testsuite_qemu_nonzero_runtime_triage.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Final Triage Check

# Current Packet

## Just Finished

Step 6: Create Focused Follow-Up Ideas preserved the top ranked repair
families as durable source ideas under `ideas/open/`.

Created follow-up ideas:

- `ideas/open/311_rv64_ordinary_control_expression_completion.md` for the
  23-row `incomplete_control_or_expression_emission` family.
- `ideas/open/312_rv64_local_stack_slot_address_materialization.md` for the
  21-row `stack_pointer_local_slot_materialization` family.
- `ideas/open/313_rv64_external_empty_stub_policy_runtime.md` for the 59-row
  `unresolved_external_or_empty_stub_emission` family.

Each idea records the owned feature family, candidate cases, acceptance
criteria, and concrete reviewer reject signals. The external empty-stub idea is
kept as a policy/runtime initiative, not as a broad "make c-testsuite pass"
umbrella and not as the first backend repair packet.

## Suggested Next

Start Step 7 from `plan.md`: run the final triage check. Verify the latest
220-case counts and artifact provenance are recorded, all runtime failures are
covered by reproducible buckets, representative evidence exists for each
non-trivial bucket, exit classes remain explicitly separated, the follow-up
ideas exist, and generated bulk artifacts stayed out of the repo root.

## Watchouts

This active idea remains triage-only. Step 7 should decide whether the triage
source idea is ready for lifecycle closure; it should not implement the follow-up
repairs. The ranking intentionally prefers semantic backend families with
focused proof surfaces over the largest external/stub count. Several external
rows also exercise arrays, loops, or pointers, so the external policy idea
should stay separate from ordinary backend lowering repairs.

## Proof

No build or CTest proof required for this lifecycle-only idea creation update.
Proof was read-only inspection of `todo.md`,
`build/rv64_c_testsuite_probe_latest/triage_step4/summary.md`, and
`build/rv64_c_testsuite_probe_latest/triage_step4/classification.tsv`, followed
by creation of the focused follow-up source ideas. No `test_after.log` was
produced because the delegated proof explicitly did not require build/ctest
proof.
