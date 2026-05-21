Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture Current Backend Surface

# Current Packet

## Just Finished

Step 1: Capture Current Backend Surface. Recorded the current backend-regex
surface from existing `test_after.log`: 357 tests selected, 351 passed, 6
failed. The local backend/unit surface is clean; every residual failure is an
`c_testsuite_aarch64_backend_*` case:

- `c_testsuite_aarch64_backend_src_00174_c` failed
- `c_testsuite_aarch64_backend_src_00187_c` failed
- `c_testsuite_aarch64_backend_src_00200_c` timed out
- `c_testsuite_aarch64_backend_src_00205_c` failed
- `c_testsuite_aarch64_backend_src_00207_c` timed out
- `c_testsuite_aarch64_backend_src_00216_c` failed

## Suggested Next

Execute Step 2: classify the six residual AArch64 backend c-testsuite buckets
without implementation. Use the current surface to separate runtime mismatch
cases (`00174`, `00187`, `00205`), timeout cases (`00200`, `00207`), and the
runtime nonzero case (`00216`) into focused-owner recommendations.

## Watchouts

This umbrella is for classification and focused-owner selection only. Do not
implement fixes under idea 295. Do not reopen closed owners from counts alone;
require generated-code, diagnostic, timeout, or proof evidence that
contradicts their closure boundary. The residual surface no longer includes
`00218`; keep Step 2 focused on the six current residual buckets above.

## Proof

No new test run per packet. Used existing `test_after.log` from:

```sh
{ cmake --build --preset default && ctest --test-dir build -j10 -R backend --output-on-failure; } > test_after.log 2>&1
```

The log shows `ninja: no work to do.`, 357 selected tests, 351 passing tests,
and 6 failures, all under the `aarch64_backend c_testsuite` labels.
