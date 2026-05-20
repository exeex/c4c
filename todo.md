Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Residual Failures

# Current Packet

## Just Finished

Step 1 captured the current post-337 main-build backend regex inventory from
`ctest --test-dir build -j10 -R backend --output-on-failure`.

Inventory from `test_after.log`:

- Selected: 354 tests
- Passed: 325 tests
- Failed: 26 tests
- Timed out: 3 tests
- Skipped: 0 tests
- Local/internal split visible from labels and test names: 142 selected, 142
  passed, 0 failed, 0 timed out, 0 skipped
- External `c_testsuite_aarch64_backend_*` split visible from labels and test
  names: 212 selected, 183 passed, 26 failed, 3 timed out, 0 skipped

Current failing tests:

- `c_testsuite_aarch64_backend_src_00086_c` (Failed)
- `c_testsuite_aarch64_backend_src_00111_c` (Failed)
- `c_testsuite_aarch64_backend_src_00112_c` (Failed)
- `c_testsuite_aarch64_backend_src_00119_c` (Failed)
- `c_testsuite_aarch64_backend_src_00123_c` (Failed)
- `c_testsuite_aarch64_backend_src_00140_c` (Failed)
- `c_testsuite_aarch64_backend_src_00143_c` (Failed)
- `c_testsuite_aarch64_backend_src_00144_c` (Failed)
- `c_testsuite_aarch64_backend_src_00157_c` (Failed)
- `c_testsuite_aarch64_backend_src_00159_c` (Failed)
- `c_testsuite_aarch64_backend_src_00170_c` (Failed)
- `c_testsuite_aarch64_backend_src_00171_c` (Failed)
- `c_testsuite_aarch64_backend_src_00173_c` (Failed)
- `c_testsuite_aarch64_backend_src_00174_c` (Failed)
- `c_testsuite_aarch64_backend_src_00175_c` (Failed)
- `c_testsuite_aarch64_backend_src_00176_c` (Failed)
- `c_testsuite_aarch64_backend_src_00181_c` (Failed)
- `c_testsuite_aarch64_backend_src_00182_c` (Failed)
- `c_testsuite_aarch64_backend_src_00185_c` (Failed)
- `c_testsuite_aarch64_backend_src_00186_c` (Failed)
- `c_testsuite_aarch64_backend_src_00187_c` (Timeout)
- `c_testsuite_aarch64_backend_src_00195_c` (Failed)
- `c_testsuite_aarch64_backend_src_00200_c` (Failed)
- `c_testsuite_aarch64_backend_src_00204_c` (Failed)
- `c_testsuite_aarch64_backend_src_00205_c` (Failed)
- `c_testsuite_aarch64_backend_src_00207_c` (Timeout)
- `c_testsuite_aarch64_backend_src_00216_c` (Failed)
- `c_testsuite_aarch64_backend_src_00218_c` (Failed)
- `c_testsuite_aarch64_backend_src_00220_c` (Timeout)

## Suggested Next

Delegate Step 2 (`Classify Residual Failures`) as a classification-only packet:
use `test_after.log` to split the residual failures into local-vs-external and
first-bad-stage buckets, then record the classified inventory in `todo.md`.
Do not implement, select a focused owner, or reopen a closed owner from counts
alone.

## Watchouts

- Do not implement under the umbrella inventory plan.
- Do not reopen closed owners through idea 337 from counts alone.
- Treat `test_after.log` as classification input only; owner selection needs
  generated-code, diagnostic, ABI, or lowering evidence that identifies a
  semantic capability boundary.
- Treat timeout and output-storm cases as unsafe for broad reruns unless the
  delegated command includes timeout and stale-process cleanup.
- Do not change expectations, runner behavior, unsupported classifications, or
  CTest registration to improve backend-regex counts.

## Proof

Command run:

`ctest --test-dir build -j10 -R backend --output-on-failure > test_after.log 2>&1`

Result: completed with exit code 8 because 29 of 354 selected tests failed or
timed out. Raw proof log preserved at `test_after.log`.
