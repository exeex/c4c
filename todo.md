Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reconstruct the Current Backend Regex Surface

# Current Packet

## Just Finished

Step 1 - Reconstruct the Current Backend Regex Surface completed a fresh
main-build backend regex capture after idea 369 closed. Log source:
`test_after.log` from
`cmake --build --preset default && ctest --test-dir build -j10 -R backend --output-on-failure > test_after.log 2>&1`.

Counts from the captured run:

- Selected: 355
- Passed: 337
- Failed: 16
- Timed out: 2
- Non-passing total: 18

Current non-passing tests:

- Local backend bucket: none
- External `c_testsuite_aarch64_backend_*` failed bucket:
  `c_testsuite_aarch64_backend_src_00112_c`,
  `c_testsuite_aarch64_backend_src_00119_c`,
  `c_testsuite_aarch64_backend_src_00123_c`,
  `c_testsuite_aarch64_backend_src_00140_c`,
  `c_testsuite_aarch64_backend_src_00143_c`,
  `c_testsuite_aarch64_backend_src_00157_c`,
  `c_testsuite_aarch64_backend_src_00163_c`,
  `c_testsuite_aarch64_backend_src_00174_c`,
  `c_testsuite_aarch64_backend_src_00176_c`,
  `c_testsuite_aarch64_backend_src_00182_c`,
  `c_testsuite_aarch64_backend_src_00183_c`,
  `c_testsuite_aarch64_backend_src_00187_c`,
  `c_testsuite_aarch64_backend_src_00205_c`,
  `c_testsuite_aarch64_backend_src_00216_c`,
  `c_testsuite_aarch64_backend_src_00218_c`,
  `c_testsuite_aarch64_backend_src_00204_c`
- External `c_testsuite_aarch64_backend_*` timed-out bucket:
  `c_testsuite_aarch64_backend_src_00200_c`,
  `c_testsuite_aarch64_backend_src_00207_c`

## Suggested Next

Run Step 2 classification against the 18 external AArch64 c-testsuite
residuals in `test_after.log`, separating first bad semantic/generated-code
owners without making implementation or expectation edits.

## Watchouts

This umbrella is classification-only. Do not implement fixes, weaken test
contracts, change runner/CTest behavior, or reopen closed owners from counts
alone. The Step 1 capture has no local backend non-passing tests; all current
residuals are external AArch64 c-testsuite entries, with two timeout cases.

## Proof

Ran:
`cmake --build --preset default && ctest --test-dir build -j10 -R backend --output-on-failure > test_after.log 2>&1`

Result: build succeeded, CTest completed with expected non-passing residuals
for inventory capture: 337/355 passed, 16 failed, 2 timed out. Proof log:
`test_after.log`.
