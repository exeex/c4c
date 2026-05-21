Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture Current Backend Surface

# Current Packet

## Just Finished

Completed Step 1: Capture Current Backend Surface for idea 295 from the
existing backend proof log.

Current backend-regex surface in `test_after.log`:

- 357 backend-selected tests ran.
- 352 tests passed.
- 5 tests failed.
- Local backend/unit tests are clean; every residual failure is under
  `c_testsuite_aarch64_backend_*`.
- Failed residual tests:
  - `c_testsuite_aarch64_backend_src_00174_c` failed.
  - `c_testsuite_aarch64_backend_src_00187_c` failed.
  - `c_testsuite_aarch64_backend_src_00200_c` timed out.
  - `c_testsuite_aarch64_backend_src_00207_c` timed out.
  - `c_testsuite_aarch64_backend_src_00216_c` failed.
- `c_testsuite_aarch64_backend_src_00205_c` and
  `c_testsuite_aarch64_backend_src_00218_c` are absent from the residual
  failure list; both appear as passing in the captured run.

## Suggested Next

Proceed to Step 2 classification only: group the five residual
`c_testsuite_aarch64_backend_*` failures by observed symptom and likely owner
boundary, without implementation.

## Watchouts

This umbrella is for classification and focused-owner selection only. Do not
implement fixes under idea 295. Do not reopen closed owners from counts alone;
require generated-code, diagnostic, timeout, or proof evidence that
contradicts their closure boundary. Keep timeout-only cases quarantined unless
a safe timeout-specific owner is explicitly selected.

## Proof

No new test run was requested for this documentation-only packet. Used the
existing `test_after.log` from:

```sh
{ cmake --build --preset default && ctest --test-dir build -j10 -R backend --output-on-failure; } > test_after.log 2>&1
```

The captured backend subset reports `99% tests passed, 5 tests failed out of
357`, with 352 passing tests and the five residual failures listed above.
