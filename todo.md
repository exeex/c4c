Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Current Failure Families

# Current Packet

## Just Finished

Step 1: Capture Fresh Backend Regex Inventory.

Fresh main-build backend regex inventory captured with the delegated proof
command. CTest selected 354 backend-matching tests: 334 passed, 18 failed
non-timeout, and 2 timed out, for 20 total non-passing tests.

Failure split:

- Local backend/unit/CLI tests: 142 selected, 142 passed, 0 failed, 0 timed out.
- `c_testsuite_aarch64_backend_*`: 212 selected, 192 passed, 18 failed, 2 timed
  out.

The only non-passing tests in the fresh run are
`c_testsuite_aarch64_backend_src_00005_c`,
`c_testsuite_aarch64_backend_src_00020_c`,
`c_testsuite_aarch64_backend_src_00103_c`,
`c_testsuite_aarch64_backend_src_00112_c`,
`c_testsuite_aarch64_backend_src_00119_c`,
`c_testsuite_aarch64_backend_src_00123_c`,
`c_testsuite_aarch64_backend_src_00130_c`,
`c_testsuite_aarch64_backend_src_00168_c`,
`c_testsuite_aarch64_backend_src_00173_c` timeout,
`c_testsuite_aarch64_backend_src_00174_c`,
`c_testsuite_aarch64_backend_src_00176_c`,
`c_testsuite_aarch64_backend_src_00181_c`,
`c_testsuite_aarch64_backend_src_00182_c`,
`c_testsuite_aarch64_backend_src_00186_c`,
`c_testsuite_aarch64_backend_src_00187_c`,
`c_testsuite_aarch64_backend_src_00195_c`,
`c_testsuite_aarch64_backend_src_00200_c`,
`c_testsuite_aarch64_backend_src_00205_c`,
`c_testsuite_aarch64_backend_src_00207_c` timeout, and
`c_testsuite_aarch64_backend_src_00216_c`.

## Suggested Next

Supervisor should choose the Step 2 classification packet from the fresh
`test_after.log` inventory before assigning any implementation work.

## Watchouts

- Do not implement fixes under the umbrella inventory plan.
- Do not reopen closed or parked owners from failing counts alone.
- The delegated command wrote only the CTest portion to `test_after.log`; the
  preceding build completed separately with `ninja: no work to do`.
- The shell pipeline exited successfully because `tee` was last in the pipeline,
  but CTest reported residual backend failures in the captured log.
- Local backend/unit/CLI tests are clean in this inventory; all residuals are
  external `c_testsuite_aarch64_backend_*` cases.
- Keep timeout cases `00173.c` and `00207.c` bounded before trusting broad
  runtime logs.

## Proof

`cmake --build --preset default && ctest --test-dir build -j10 -R backend --output-on-failure | tee test_after.log`

Build completed. Backend CTest inventory completed with expected residual
failures captured in `test_after.log`.
