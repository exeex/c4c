Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture Current Backend Regex Inventory

# Current Packet

## Just Finished

Step 1: Capture Current Backend Regex Inventory completed the current
post-338 main-build backend regex inventory surface without implementation
changes. The delegated backend proof selected 354 tests; the visible CTest
summary reports 327 passed, 27 total failed out of 354, with 23 `Failed`
entries and 4 `Timeout` entries. No skipped count is shown in
`test_after.log`.

## Suggested Next

Delegate the next inventory-analysis packet to group the 27
`c_testsuite_aarch64_backend_*` failures by the shallow surface visible in
`test_after.log`, without touching implementation or expectations.

## Watchouts

- Do not implement under the umbrella inventory plan.
- Do not reopen closed owners through idea 338 from counts alone.
- Treat timeout and output-storm cases as unsafe for broad reruns unless the
  delegated command includes timeout and stale-process cleanup.
- Do not change expectations, runner behavior, unsupported classifications, or
  CTest registration to improve backend-regex counts.
- All failures in this Step 1 log are `c_testsuite_aarch64_backend_*` entries;
  no local `backend_*` tests appear in the failed list.
- Timeout entries are `src_00173_c`, `src_00187_c`, `src_00207_c`, and
  `src_00220_c`.

## Proof

Ran exactly:

```sh
cmake --build build --target c4cll -j 2 && (ctest --test-dir build -j10 -R backend --output-on-failure > test_after.log 2>&1 || true)
```

Build reported `ninja: no work to do.` CTest wrote `test_after.log` and
reported `92% tests passed, 27 tests failed out of 354`; selected 354, passed
327, failed 23, timed out 4, skipped 0 visible in the log. The failed-list
surface contains only `c_testsuite_aarch64_backend_*` tests, not local backend
tests. Proof log path: `test_after.log`.
