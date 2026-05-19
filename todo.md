# Backend Regex Failure Family Inventory Todo

Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture Current Backend Regex Inventory

# Current Packet

Use `ctest -R backend` from the main `/workspaces/c4c/build` tree as the
inventory source. Do not implement fixes in this umbrella plan.

## Just Finished

- Step 1: captured the current main-build backend regex inventory from
  `/workspaces/c4c/build`.
- Fresh run selected 352 backend-regex tests: 272 passed and 80 failed.
- All 80 failures were in `c_testsuite_aarch64_backend_*`; the final failed
  list includes 79 `Failed` results and 1 `Timeout`
  (`c_testsuite_aarch64_backend_src_00220_c`).
- Stale runtime process check after the run found no lingering
  `qemu-aarch64`, `c_testsuite_aarch64`, `c-testsuite`, or external
  c-testsuite runtime processes.

## Suggested Next

- Classify the 80 captured c-testsuite AArch64 backend failures in
  `/workspaces/c4c/test_after.log` by failure source/family without changing
  implementation, expectations, allowlists, unsupported classifications, CTest
  registration, timeout policy, or runner behavior.
- Before switching away from this umbrella, write durable findings back into
  `ideas/open/295_backend_regex_failure_family_inventory.md` under
  `## Deactivation Note`, following the idea 284 style.

## Watchouts

- Do not treat all `backend` regex failures as one owner.
- Keep local backend/unit regressions separate from AArch64 external
  c-testsuite runtime failures.
- Do not change expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, or CTest registration to improve counts.
- Do not reopen closed AArch64 owners 285 through 294 from counts alone.

## Proof

Inventory command:

```bash
cd /workspaces/c4c/build && ctest -j10 -R backend --output-on-failure | tee /workspaces/c4c/test_after.log
```

Result: 77% tests passed, 80 tests failed out of 352. Proof log:
`/workspaces/c4c/test_after.log`.

Stale-process check:

```bash
pgrep -af 'qemu-aarch64|c_testsuite_aarch64|c-testsuite|tests/c/external|/workspaces/c4c/build/.*/c_testsuite|/workspaces/c4c/tests/c/external' || true
```

Result: no stale runtime process found; the only match was the probe command
itself.
