# Backend Regex Failure Family Inventory Todo

Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh The Remaining Backend Surface

# Current Packet

## Just Finished

Step 1: Refresh The Remaining Backend Surface.

Refreshed the main-build backend-regex inventory from `/workspaces/c4c/build`
after closed owner 296, without implementation or expectation changes. The
delegated command selected 352 tests and reported 290 passed / 62 failed
(including one timeout) in `/workspaces/c4c/test_after.log`.

Failing c-testsuite AArch64 backend sources:
`00024`, `00031`, `00035`, `00046`, `00050`, `00064`, `00066`, `00086`,
`00089`, `00102`, `00104`, `00105`, `00111`, `00112`, `00113`, `00119`,
`00121`, `00123`, `00126`, `00134`, `00135`, `00137`, `00138`, `00139`,
`00140`, `00143`, `00144`, `00151`, `00157`, `00159`, `00164`, `00168`,
`00169`, `00170`, `00172`, `00173`, `00174`, `00175`, `00176`, `00179`,
`00181`, `00182`, `00185`, `00186`, `00187`, `00189`, `00193`, `00194`,
`00195`, `00196`, `00200`, `00204`, `00205`, `00207`, `00208`, `00209`,
`00213`, `00214`, `00215`, `00216`, `00218`, `00220`.

## Suggested Next

Classify the refreshed failing list by failure source for Step 2, preserving
closed-owner boundaries 285 through 296 unless generated-code or proof evidence
contradicts them.

## Watchouts

- `00200` is currently classified as a runtime mismatch.
- `00207`, `00214`, and `00215` are currently classified as scalar add/xor
  immediate printer limits.
- `00220` timed out in the refreshed broad backend-regex run.
- Stale-process check after the run found no leftover `qemu`, `c_testsuite`, or
  AArch64 runtime test processes; only the current Codex process path and the
  `rg` process used for the check matched `aarch64`.
- Do not reopen closed owners 285 through 296 from counts alone.
- Do not implement backend fixes while this umbrella inventory is active.
- Do not change expectations, allowlists, unsupported classifications, CTest
  registration, timeout policy, or runner behavior.

## Proof

Ran exactly:

`cd /workspaces/c4c/build && ctest -j10 -R backend --output-on-failure | tee /workspaces/c4c/test_after.log`

Result: inventory refreshed. CTest selected 352 tests and reported 82% tests
passed, 62 tests failed out of 352. Proof log: `/workspaces/c4c/test_after.log`.

Stale-process check:

`ps -eo pid,ppid,stat,comm,args | rg -i 'qemu|c_testsuite|aarch64' || true`

Result: no leftover `qemu`, `c_testsuite`, or AArch64 runtime test processes.
