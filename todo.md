Status: Active
Source Idea Path: ideas/open/06_prepared_call_preservation_source_authority.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Validate Representative Call Paths

# Current Packet

## Just Finished

Completed Step 5 of `plan.md`: repaired the AArch64 c_testsuite call-path
validation failures by keeping register-backed pointer arguments on their
prepared register source authority instead of publishing false
`LocalFrameAddressMaterialization` source selections, while preserving real
prepared `PriorPreservation` selections for later calls.

The prepared call-plan builder now maintains an incremental prior-preservation
lookup while walking calls, avoiding the per-call full lookup rebuild that made
`00200.c` exceed the CTest case timeout.

## Suggested Next

Supervisor can review and commit the Step 5 code plus `todo.md` slice.

## Watchouts

- The repair does not reintroduce AArch64 target-local prior-source scanning;
  prior-preserved call arguments still require prepared `PriorPreservation`
  source selections.
- `00200.c` was a CTest compile-time timeout after the runtime repair; the
  incremental prepared lookup brings the delegated subset back under the case
  timeout.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00077_c|c_testsuite_aarch64_backend_src_00078_c|c_testsuite_aarch64_backend_src_00173_c|c_testsuite_aarch64_backend_src_00179_c|c_testsuite_aarch64_backend_src_00180_c|c_testsuite_aarch64_backend_src_00182_c|c_testsuite_aarch64_backend_src_00186_c|c_testsuite_aarch64_backend_src_00187_c|c_testsuite_aarch64_backend_src_00200_c|c_testsuite_aarch64_backend_src_00204_c|c_testsuite_aarch64_backend_src_00216_c)$'`

Result: passed. The build was already up to date and the delegated AArch64
c_testsuite backend subset passed 11/11.
Proof log: `test_after.log`.

Supervisor follow-up validation:

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_'`
  passed 220/220.
- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
  passed 162/162.
