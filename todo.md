Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture Current Backend Regex Surface

# Current Packet

## Just Finished

Step 1 captured the current main-build backend regex surface from
`test_after.log`.

- Command/log source:
  `ctest --test-dir build -j10 -R backend --output-on-failure | tee test_after.log`
- Selected: 354 tests.
- Passed: 337 tests.
- Non-passing: 17 tests total; 15 failed and 2 timed out.
- Local backend/unit/CLI surface: clean. The selected local/internal backend,
  backend route, CLI, MIR, and prepared-handoff tests passed.
- External `c_testsuite_aarch64_backend_*` surface: 212 selected, 195 passed,
  17 non-passing.
- External failure buckets observed in the log: 2 `FRONTEND_FAIL`, 9
  `RUNTIME_NONZERO`, 4 `RUNTIME_MISMATCH`, and 2 timeout cases.

## Suggested Next

Have the supervisor route Step 2 to compare this captured surface against the
active idea and choose a focused semantic owner. Do not implement from this
classification packet alone.

## Watchouts

- This umbrella is classification-only; do not edit implementation files or
  tests here.
- Do not reopen closed or parked focused owners from pass counts alone.
- Both timeout cases are external c-testsuite entries:
  `c_testsuite_aarch64_backend_src_00173_c` and
  `c_testsuite_aarch64_backend_src_00207_c`.
- All 17 non-passing entries are external
  `c_testsuite_aarch64_backend_*` cases; no local backend/unit/CLI failure was
  observed in this capture.

## Proof

`ctest --test-dir build -j10 -R backend --output-on-failure | tee test_after.log`

Result: completed with expected non-clean external surface, 337/354 passing.
Proof log: `test_after.log`.
