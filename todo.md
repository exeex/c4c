Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture Current Backend Surface

# Current Packet

## Just Finished

Step 1 captured the current backend-regex residual surface after closed idea
378. The delegated backend proof selected 358 tests: 355 passed, 3 failed
overall, with 1 non-timeout runtime failure and 2 timeouts. Local backend/unit
tests are clean; the residual failures are limited to
`c_testsuite_aarch64_backend_src_00216_c` failing at runtime with
`RUNTIME_NONZERO`/segmentation fault, plus timeout-only
`c_testsuite_aarch64_backend_src_00200_c` and
`c_testsuite_aarch64_backend_src_00207_c`. `00174` is absent from failures and
passed in this capture.

## Suggested Next

Classify `c_testsuite_aarch64_backend_src_00216_c` as the next non-timeout
backend residual bucket. Keep `00200` and `00207` quarantined as timeout-only
unless the supervisor selects a timeout-specific owner.

## Watchouts

This umbrella is for classification and focused-owner selection only. Do not
implement fixes under idea 295. Do not reopen closed owners from counts alone;
require generated-code, diagnostic, timeout, or proof evidence that
contradicts their closure boundary. The latest capture directly confirms that
`00174` remains closed for the backend surface.

## Proof

Ran the delegated capture command and preserved output in `test_after.log`:

```sh
{ cmake --build --preset default && ctest --test-dir build -j10 -R backend --output-on-failure; } > test_after.log 2>&1
```

The command exited 8 because CTest reported the expected residual backend
failures; it was sufficient for this classification-only Step 1 capture.
