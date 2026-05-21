Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Residual Buckets

# Current Packet

## Just Finished

Executed Step 1 backend-regex surface capture after closing idea 371. The
previous `test_after.log` was the narrow idea 371 refinement proof, so it was
not sufficient for the umbrella residual surface; refreshed it with the
delegated backend regex command.

Backend-regex result: 356 tests selected, 347 passed, 9 failed. Of the 9
failures, 7 are non-timeout failures and 2 are timeouts. The residual external
AArch64 backend tests are:

- Runtime/failure: `00163`, `00174`, `00182`, `00187`, `00205`, `00216`,
  `00218`.
- Timeout: `00200`, `00207`.

No local `backend_*` unit/route/MIR/BIR/CLI/runtime/smoke tests failed in this
run. The residual surface is only external AArch64 backend C tests. Recently
resolved idea 371 representatives remain off the failure list: `00157`,
`00176`, and `00183` pass.

## Suggested Next

Execute Step 2 classification on the 9 remaining external AArch64 backend
residuals. Suggested first pass: group timeout representatives `00200`/`00207`
separately, then inspect leading runtime/failure representatives `00163`,
`00174`, and `00182` before deciding the next focused owner.

## Watchouts

This umbrella is for classification and focused-owner selection only. Do not
implement fixes under idea 295. Do not reopen closed owners from counts alone;
require generated-code, diagnostic, timeout, or proof evidence that
contradicts their closure boundary. Reclassify the parked buckets after
removing the resolved `00157`, `00176`, and `00183` facts from the current
surface. `00187` remains on the residual list even though related closed idea
348 exists; classify from current first bad fact before assigning ownership.

## Proof

Delegated Step 1 command:

```sh
cmake --build --preset default && ctest --test-dir build -j10 -R backend --output-on-failure > test_after.log 2>&1
```

Result: build completed; CTest selected 356 tests, 347 passed, 9 failed.
Failures were `00163`, `00174`, `00182`, `00187`, `00200` timeout, `00205`,
`00207` timeout, `00216`, and `00218`. Proof log is `test_after.log`.
