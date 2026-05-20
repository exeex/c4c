Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture Fresh Backend Regex Inventory

# Current Packet

## Just Finished

Step 1: Capture Fresh Backend Regex Inventory. Captured the supervisor-selected
main-build backend-regex CTest surface in `test_after.log` without
implementation or test edits.

Selected inventory:

- Total selected: 353 tests.
- Passed: 300 tests.
- Failed: 53 tests.
- Skipped: 0 visible in the CTest summary.
- Timeout: 3 tests.
- Incomplete: 0 visible; the command completed and wrote `test_after.log`.

First-pass split:

- Local backend/unit/CLI tests: 141 selected, 141 passed, 0 failed.
- `c_testsuite_aarch64_backend_*`: 212 selected, 159 passed, 53 failed.

Failing `c_testsuite_aarch64_backend_*` tests by visible mode:

- `RUNTIME_NONZERO` (29): `c_testsuite_aarch64_backend_src_00004_c`,
  `c_testsuite_aarch64_backend_src_00014_c`,
  `c_testsuite_aarch64_backend_src_00013_c`,
  `c_testsuite_aarch64_backend_src_00077_c`,
  `c_testsuite_aarch64_backend_src_00022_c`,
  `c_testsuite_aarch64_backend_src_00052_c`,
  `c_testsuite_aarch64_backend_src_00016_c`,
  `c_testsuite_aarch64_backend_src_00011_c`,
  `c_testsuite_aarch64_backend_src_00019_c`,
  `c_testsuite_aarch64_backend_src_00020_c`,
  `c_testsuite_aarch64_backend_src_00086_c`,
  `c_testsuite_aarch64_backend_src_00103_c`,
  `c_testsuite_aarch64_backend_src_00087_c`,
  `c_testsuite_aarch64_backend_src_00112_c`,
  `c_testsuite_aarch64_backend_src_00089_c`,
  `c_testsuite_aarch64_backend_src_00111_c`,
  `c_testsuite_aarch64_backend_src_00117_c`,
  `c_testsuite_aarch64_backend_src_00116_c`,
  `c_testsuite_aarch64_backend_src_00118_c`,
  `c_testsuite_aarch64_backend_src_00119_c`,
  `c_testsuite_aarch64_backend_src_00124_c`,
  `c_testsuite_aarch64_backend_src_00121_c`,
  `c_testsuite_aarch64_backend_src_00139_c`,
  `c_testsuite_aarch64_backend_src_00123_c`,
  `c_testsuite_aarch64_backend_src_00144_c`,
  `c_testsuite_aarch64_backend_src_00153_c`,
  `c_testsuite_aarch64_backend_src_00140_c`,
  `c_testsuite_aarch64_backend_src_00208_c`,
  `c_testsuite_aarch64_backend_src_00200_c`.
- `RUNTIME_MISMATCH` (12): `c_testsuite_aarch64_backend_src_00158_c`,
  `c_testsuite_aarch64_backend_src_00157_c`,
  `c_testsuite_aarch64_backend_src_00159_c`,
  `c_testsuite_aarch64_backend_src_00169_c`,
  `c_testsuite_aarch64_backend_src_00168_c`,
  `c_testsuite_aarch64_backend_src_00171_c`,
  `c_testsuite_aarch64_backend_src_00170_c`,
  `c_testsuite_aarch64_backend_src_00174_c`,
  `c_testsuite_aarch64_backend_src_00186_c`,
  `c_testsuite_aarch64_backend_src_00218_c`,
  `c_testsuite_aarch64_backend_src_00182_c`,
  `c_testsuite_aarch64_backend_src_00195_c`.
- `FRONTEND_FAIL` / AArch64 machine-node printer failure (9):
  `c_testsuite_aarch64_backend_src_00143_c`,
  `c_testsuite_aarch64_backend_src_00173_c`,
  `c_testsuite_aarch64_backend_src_00175_c`,
  `c_testsuite_aarch64_backend_src_00185_c`,
  `c_testsuite_aarch64_backend_src_00181_c`,
  `c_testsuite_aarch64_backend_src_00176_c`,
  `c_testsuite_aarch64_backend_src_00205_c`,
  `c_testsuite_aarch64_backend_src_00216_c`,
  `c_testsuite_aarch64_backend_src_00204_c`.
- `Timeout` (3): `c_testsuite_aarch64_backend_src_00187_c`,
  `c_testsuite_aarch64_backend_src_00220_c`,
  `c_testsuite_aarch64_backend_src_00207_c`.

Visible `RUNTIME_NONZERO` exit buckets: `exit=1` (10), `exit=192` (4),
`exit=224` (2), `exit=240` (2), `Segmentation fault` (2), and one each for
`exit=2`, `exit=32`, `exit=48`, `exit=64`, `exit=80`, `exit=128`, `exit=140`,
`exit=144`, and `Bus error`.

## Suggested Next

Execute Step 2 by classifying the 53 `c_testsuite_aarch64_backend_*` failures
into semantic owner candidates using current logs and representative generated
artifacts. Start from the visible mode buckets above, but do not split an owner
from filename/count evidence alone.

## Watchouts

The fresh inventory shows no local backend/unit/CLI failures in the selected
regex. All current failures are external AArch64 c-testsuite cases. The printer
bucket repeatedly reports `cannot print AArch64 machine node family=scalar`
with `sign_extend` or `zero_extend` requiring a structured register source
operand, but this packet did not inspect generated artifacts deeply enough to
name an owner. Leave unrelated transient files under `review/` untouched.

## Proof

Ran:

`ctest --test-dir build -j --output-on-failure -R '^backend_|c_testsuite_aarch64_backend' > test_after.log 2>&1`

Result: command exited 8 after completing the selected inventory. CTest
reported `85% tests passed, 53 tests failed out of 353`; all 53 failures are
`c_testsuite_aarch64_backend_*` failures. Proof log: `test_after.log`.
