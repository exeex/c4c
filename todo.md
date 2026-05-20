Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture Fresh Backend Regex Inventory

# Current Packet

## Just Finished

Step 1 captured the fresh main-build backend-regex inventory into
`test_after.log` with:

```bash
ctest --test-dir build -j --output-on-failure -R 'backend' > test_after.log 2>&1
```

Current CTest counts:

- selected: `354`
- passed: `299`
- failed total: `55`
- ordinary failed status: `51`
- timeout: `4`
- skipped/not run: `0`
- incomplete: `0`
- total real time: `5.58 sec`

First-pass split:

- local backend/unit/CLI and other non-c-testsuite backend-regex tests:
  `142` selected, `142` passed, `0` failed
- `c_testsuite_aarch64_backend_*`: `212` selected, `157` passed, `55` failed

Failure-mode buckets from `test_after.log`:

- `FRONTEND_FAIL` (`2`): `c_testsuite_aarch64_backend_src_00164_c`,
  `c_testsuite_aarch64_backend_src_00214_c`
- `RUNTIME_MISMATCH` (`17`):
  `c_testsuite_aarch64_backend_src_00157_c`,
  `c_testsuite_aarch64_backend_src_00158_c`,
  `c_testsuite_aarch64_backend_src_00159_c`,
  `c_testsuite_aarch64_backend_src_00168_c`,
  `c_testsuite_aarch64_backend_src_00169_c`,
  `c_testsuite_aarch64_backend_src_00170_c`,
  `c_testsuite_aarch64_backend_src_00171_c`,
  `c_testsuite_aarch64_backend_src_00174_c`,
  `c_testsuite_aarch64_backend_src_00175_c`,
  `c_testsuite_aarch64_backend_src_00176_c`,
  `c_testsuite_aarch64_backend_src_00182_c`,
  `c_testsuite_aarch64_backend_src_00185_c`,
  `c_testsuite_aarch64_backend_src_00186_c`,
  `c_testsuite_aarch64_backend_src_00195_c`,
  `c_testsuite_aarch64_backend_src_00205_c`,
  `c_testsuite_aarch64_backend_src_00216_c`,
  `c_testsuite_aarch64_backend_src_00218_c`
- `RUNTIME_NONZERO` (`32`):
  `c_testsuite_aarch64_backend_src_00004_c`,
  `c_testsuite_aarch64_backend_src_00011_c`,
  `c_testsuite_aarch64_backend_src_00013_c`,
  `c_testsuite_aarch64_backend_src_00014_c`,
  `c_testsuite_aarch64_backend_src_00015_c`,
  `c_testsuite_aarch64_backend_src_00016_c`,
  `c_testsuite_aarch64_backend_src_00018_c`,
  `c_testsuite_aarch64_backend_src_00019_c`,
  `c_testsuite_aarch64_backend_src_00020_c`,
  `c_testsuite_aarch64_backend_src_00022_c`,
  `c_testsuite_aarch64_backend_src_00052_c`,
  `c_testsuite_aarch64_backend_src_00077_c`,
  `c_testsuite_aarch64_backend_src_00086_c`,
  `c_testsuite_aarch64_backend_src_00087_c`,
  `c_testsuite_aarch64_backend_src_00089_c`,
  `c_testsuite_aarch64_backend_src_00103_c`,
  `c_testsuite_aarch64_backend_src_00111_c`,
  `c_testsuite_aarch64_backend_src_00112_c`,
  `c_testsuite_aarch64_backend_src_00116_c`,
  `c_testsuite_aarch64_backend_src_00117_c`,
  `c_testsuite_aarch64_backend_src_00118_c`,
  `c_testsuite_aarch64_backend_src_00119_c`,
  `c_testsuite_aarch64_backend_src_00121_c`,
  `c_testsuite_aarch64_backend_src_00123_c`,
  `c_testsuite_aarch64_backend_src_00124_c`,
  `c_testsuite_aarch64_backend_src_00139_c`,
  `c_testsuite_aarch64_backend_src_00140_c`,
  `c_testsuite_aarch64_backend_src_00144_c`,
  `c_testsuite_aarch64_backend_src_00153_c`,
  `c_testsuite_aarch64_backend_src_00181_c`,
  `c_testsuite_aarch64_backend_src_00200_c`,
  `c_testsuite_aarch64_backend_src_00208_c`
- `Timeout` (`4`): `c_testsuite_aarch64_backend_src_00173_c`,
  `c_testsuite_aarch64_backend_src_00187_c`,
  `c_testsuite_aarch64_backend_src_00207_c`,
  `c_testsuite_aarch64_backend_src_00220_c`

Notable exact nonzero exits visible in this first-pass capture:

- `c_testsuite_aarch64_backend_src_00140_c`: `RUNTIME_NONZERO`, exit
  `Segmentation fault`
- `c_testsuite_aarch64_backend_src_00181_c`: `RUNTIME_NONZERO`, exit
  `Segmentation fault`
- `c_testsuite_aarch64_backend_src_00208_c`: `RUNTIME_NONZERO`, exit
  `Segmentation fault`

## Suggested Next

Execute Step 2 by classifying the current `c_testsuite_aarch64_backend_*`
failure families from `test_after.log` and current generated artifacts. Start
with the two `FRONTEND_FAIL` cases, the 17 `RUNTIME_MISMATCH` cases, the 32
`RUNTIME_NONZERO` cases, and the four timeout cases as separate first-pass
buckets; do not split or activate a focused idea until generated-code or
diagnostic evidence supports a semantic owner.

## Watchouts

Do not edit implementation files or tests under this umbrella idea. Do not
reactivate parked ideas 316, 328, 329, or 332 from stale lifecycle notes; require
fresh generated-code, diagnostic, or proof evidence that their exact owner has
returned. Keep the unrelated transient
`review/326_stdarg_byval_route_review.md` untouched.

This packet did not perform deep owner classification and did not create or
modify source ideas. The first-pass inventory shows no local backend/unit/CLI
failures selected by the backend regex; all current failures are external
AArch64 c-testsuite backend tests.

## Proof

Ran the delegated proof command exactly:

```bash
ctest --test-dir build -j --output-on-failure -R 'backend' > test_after.log 2>&1
```

CTest exited nonzero because the inventory contains expected failing tests.
`test_after.log` is the canonical proof log for this packet.
