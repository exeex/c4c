Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture Fresh Backend Regex Inventory

# Current Packet

## Just Finished

Step 1 - Capture Fresh Backend Regex Inventory is complete. The fresh
main-build backend regex run selected 354 tests: 333 passed, 21 failed, and 2
of the failed tests timed out.

Local backend/unit/CLI coverage selected by `-R backend` is clean in this run;
all listed failures are external `c_testsuite_aarch64_backend_*` registrations.
The failing external set is:
`00005`, `00020`, `00103`, `00112`, `00119`, `00123`, `00130`, `00168`,
`00173` timeout, `00174`, `00176`, `00181`, `00182`, `00186`, `00187`,
`00195`, `00200`, `00204`, `00205`, `00207` timeout, and `00216`.

## Suggested Next

Run Step 2 from `plan.md`: classify the current external AArch64 c-testsuite
failures by first bad fact using `test_after.log`, keeping timeout/output-storm
observations bounded and factual.

## Watchouts

- Do not implement fixes under the umbrella inventory plan.
- Do not reopen closed or parked owners from failing counts alone.
- The only timed-out cases in the fresh inventory are
  `c_testsuite_aarch64_backend_src_00173_c` and
  `c_testsuite_aarch64_backend_src_00207_c`, both at about 5 seconds.
- `ctest` reported errors, but the delegated shell pipeline used `tee` without
  `pipefail`; treat the CTest summary in `test_after.log` as authoritative.

## Proof

Ran:
`cmake --build --preset default && ctest --test-dir build -j10 -R backend --output-on-failure | tee test_after.log`

Build was up to date. CTest selected 354 tests, passed 333, failed 21, and
timed out 2. Proof log is `test_after.log`.
