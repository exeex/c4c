Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture Current Backend Regex Inventory

# Current Packet

## Just Finished

Step 1: Capture Current Backend Regex Inventory completed for the post-339
main-build backend regex surface. The delegated command selected 354 tests:
329 passed, 25 failed/non-passed total, 4 of those were CTest timeouts, and no
skipped tests were reported in `test_after.log`.

All 25 failed/non-passed tests are external
`c_testsuite_aarch64_backend_*` tests. No local backend/unit/CLI failures
appear in the failed-test summary.

## Suggested Next

Delegate Step 2 to classify the residual failures from `test_after.log` into
the runbook's inventory buckets without implementing fixes.

## Watchouts

- Do not implement under the umbrella inventory plan.
- Do not reopen closed owners through idea 339 from counts alone.
- Treat timeout and output-storm cases as unsafe for broad reruns unless the
  delegated command includes timeout and stale-process cleanup.
- Do not change expectations, runner behavior, unsupported classifications, or
  CTest registration to improve backend-regex counts.
- The failed-test summary contains only `c_testsuite_aarch64_backend_*`
  entries: 21 ordinary `Failed` results and 4 `Timeout` results.

## Proof

Exact command run:

```sh
cmake --build build --target c4cll -j 2 && (ctest --test-dir build -j10 -R backend --output-on-failure > test_after.log 2>&1 || true)
```

Canonical proof log: `test_after.log`.

CTest summary from `test_after.log`:

- Selected: 354
- Passed: 329
- Failed/non-passed total: 25
- Timeout: 4
- Skipped: 0 reported
- Local backend/unit/CLI failures: 0
- `c_testsuite_aarch64_backend_*` failed/non-passed: 25
