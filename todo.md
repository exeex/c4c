Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture Current Backend Surface

# Current Packet

## Just Finished

Executed idea 295 Step 1, capturing the current backend-regex residual surface
after idea 372 closure.

Backend-regex result:

- Selected: 356 tests.
- Passed: 348 tests.
- Failed: 8 tests total.
- Timeouts: 2 tests (`00200`, `00207`).
- Non-timeout failures: 6 tests (`00174`, `00182`, `00187`, `00205`,
  `00216`, `00218`).

All local `backend_*` unit/route/MIR/BIR/CLI/runtime/smoke tests selected by
the backend regex passed. The remaining failures are only external AArch64
backend c-testsuite tests:

- `c_testsuite_aarch64_backend_src_00174_c`
- `c_testsuite_aarch64_backend_src_00182_c`
- `c_testsuite_aarch64_backend_src_00187_c`
- `c_testsuite_aarch64_backend_src_00200_c` timeout
- `c_testsuite_aarch64_backend_src_00205_c`
- `c_testsuite_aarch64_backend_src_00207_c` timeout
- `c_testsuite_aarch64_backend_src_00216_c`
- `c_testsuite_aarch64_backend_src_00218_c`

## Suggested Next

Execute idea 295 Step 2: classify the 8 external AArch64 backend residuals by
first bad fact and semantic owner. Start with leading non-timeout reps
`00174`, `00182`, and `00187`, then inspect timeout reps `00200` and `00207`
for loop/control-owner evidence before ranking the remaining residuals.

## Watchouts

This umbrella is for classification and focused-owner selection only. Do not
implement fixes under idea 295. Do not reopen closed owners from counts alone;
require generated-code, diagnostic, timeout, or proof evidence that
contradicts their closure boundary. `00163` is absent from the current
backend-regex failure list after idea 372 closure.

## Proof

Delegated Step 1 command:

```sh
cmake --build --preset default && ctest --test-dir build -j10 -R backend --output-on-failure > test_after.log 2>&1
```

Result: build completed; CTest selected 356 backend-regex tests, 348 passed,
8 failed. The CTest command exited nonzero only because the known external
AArch64 backend residuals remain. Proof log is `test_after.log`.
