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

- Created the backend-regex umbrella source idea and active runbook.
- Confirmed `ctest --test-dir /workspaces/c4c/build -N -R backend` selects 352
  tests.
- Confirmed 212 selected tests are `c_testsuite_aarch64_backend_*`.
- User observed about 80 failures from `ctest -j10 -R backend`.

## Suggested Next

- Capture a fresh backend regex log from `/workspaces/c4c/build`:

```bash
ctest -j10 -R backend --output-on-failure
```

- Record pass/fail count and classify failures by source.
- Check for stale runtime processes after the run.
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

Inventory count command already checked:

```bash
ctest --test-dir /workspaces/c4c/build -N -R backend
```

Result: 352 tests selected, including 212
`c_testsuite_aarch64_backend_*` tests.
