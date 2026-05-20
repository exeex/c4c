Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Residual Failures Against Closed Boundaries

# Current Packet

## Just Finished

Completed `plan.md` Step 1 by reconstructing the post-345 backend-regex
residual inventory. Existing root logs were not sufficient as the canonical
inventory basis: prior `test_after.log` covered only 142 backend-selected tests,
while the delegated broad backend regex command selected 354 tests.

Inventory basis:
`cd /workspaces/c4c/build && ctest -j10 -R backend --output-on-failure > /workspaces/c4c/test_after.log 2>&1`

Result from `/workspaces/c4c/test_after.log`:
- Selected: 354
- Passed: 330
- Failed: 24 total, including 2 timeouts
- Timeout: 2

Local backend/unit/CLI residuals:
- 0 failures observed in the local backend/unit/CLI portion of the run.

`c_testsuite_aarch64_backend_*` residuals:
- 24 failures total: 22 non-timeout failures and 2 timeouts.
- Non-timeout failures: `00005.c`, `00020.c`, `00103.c`, `00112.c`,
  `00119.c`, `00123.c`, `00130.c`, `00140.c`, `00159.c`, `00170.c`,
  `00174.c`, `00175.c`, `00176.c`, `00181.c`, `00182.c`, `00186.c`,
  `00187.c`, `00195.c`, `00200.c`, `00205.c`, `00216.c`, `00218.c`.
- Timeouts: `00173.c`, `00207.c`.

## Suggested Next

Execute `plan.md` Step 2: classify the 24 `c_testsuite_aarch64_backend_*`
residuals into coherent failure families without changing implementation,
expectations, unsupported classifications, runner behavior, or timeout policy.

## Watchouts

- This umbrella is classification-only; implementation should move to a
  focused split idea before code edits begin.
- Do not reopen closed focused owners from counts alone.
- Do not treat parked close-deferred ideas as active repair targets unless
  fresh evidence proves their old owner has returned.
- The broad `-R backend` regex now selects both the local backend bucket and
  212 `c_testsuite` AArch64 backend tests; keep those classes separate in
  inventory notes.

## Proof

Ran delegated inventory command:
`cd /workspaces/c4c/build && ctest -j10 -R backend --output-on-failure > /workspaces/c4c/test_after.log 2>&1`

`test_after.log` is the canonical proof log. CTest exited nonzero because the
inventory intentionally includes current residual failures: 354 selected, 330
passed, 24 failed, 2 timed out.
