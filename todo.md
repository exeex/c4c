Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture Current Backend Inventory

# Current Packet

## Just Finished

Step 1 captured the current backend inventory from the supervisor-accepted
`test_baseline.log`.

Inventory source:

- Baseline commit: `dda380c871e37663dff6c75509833dc611c9b47e`
- Baseline subject: `Fix AArch64 close-blocker regressions`
- Scope recorded by the log: `<full-suite>`
- Result: 3375 total, 3345 passed, 30 failed

Local backend/unit status:

- No local `backend_.*`, backend unit, backend CLI, or backend route tests are
  listed in the accepted baseline failure set.
- Backend labels in the accepted run include `backend` with 143 tests and
  `aarch64_backend` with 212 tests.
- One non-backend workflow failure is present:
  `string_authority_guard`.

External AArch64 c-testsuite backend failures:

- `c_testsuite_aarch64_backend_src_00112_c` failed
- `c_testsuite_aarch64_backend_src_00143_c` failed
- `c_testsuite_aarch64_backend_src_00157_c` failed
- `c_testsuite_aarch64_backend_src_00163_c` failed
- `c_testsuite_aarch64_backend_src_00174_c` failed
- `c_testsuite_aarch64_backend_src_00176_c` failed
- `c_testsuite_aarch64_backend_src_00182_c` failed
- `c_testsuite_aarch64_backend_src_00183_c` failed
- `c_testsuite_aarch64_backend_src_00187_c` failed
- `c_testsuite_aarch64_backend_src_00200_c` timed out
- `c_testsuite_aarch64_backend_src_00205_c` failed
- `c_testsuite_aarch64_backend_src_00207_c` timed out
- `c_testsuite_aarch64_backend_src_00216_c` failed
- `c_testsuite_aarch64_backend_src_00218_c` failed

Recently repaired AArch64 representatives:

- `c_testsuite_aarch64_backend_src_00123_c`: pass in the accepted baseline
  inventory by registration plus absence from the failed list; `test_after.log`
  also records it passing in the focused context.
- `c_testsuite_aarch64_backend_src_00130_c`: pass in the accepted baseline
  inventory by registration plus absence from the failed list; `test_after.log`
  also records it passing in the focused context.
- `c_testsuite_aarch64_backend_src_00140_c`: pass in the accepted baseline
  inventory by registration plus absence from the failed list; `test_after.log`
  also records it passing in the focused context.
- `c_testsuite_aarch64_backend_src_00204_c`: pass in the accepted baseline
  inventory by registration plus absence from the failed list; `test_after.log`
  also records it passing in the focused context.

Other full-suite failures outside the AArch64 backend inventory are 15
`llvm_gcc_c_torture_*` tests:
`ieee_fp_cmp_3`, `20020413_1`, `20030914_1`, `20040208_1`, `960215_1`,
`960513_1`, `align_2`, `ieee_20010226_1`, `ieee_copysign1`,
`ieee_copysign2`, `ieee_fp_cmp_4l`, `ieee_fp_cmp_8l`, `ieee_inf_1`,
`regstack_1`, and `stdarg_2`.

## Suggested Next

Execute Step 2 from `plan.md`: classify the 14 residual
`c_testsuite_aarch64_backend_*` failures into semantic buckets, treating
`00200` and `00207` as timeout/quarantine candidates unless a bounded artifact
is already available.

## Watchouts

Evidence gaps for Step 2:

- `test_baseline.log` gives names and pass/fail/timeout state, but not first
  bad facts for the 12 non-timeout AArch64 backend failures.
- Generated assembly, prepared dumps, runner diagnostics, or focused logs are
  still needed before assigning owners to `00112`, `00143`, `00157`, `00163`,
  `00174`, `00176`, `00182`, `00183`, `00187`, `00205`, `00216`, and `00218`.
- `00200` and `00207` are timeout cases in the accepted baseline and should
  not be mixed into runtime/machine-printer buckets without bounded timeout
  evidence.
- The non-backend workflow and LLVM GCC torture failures are present in the
  full-suite baseline but should stay separate from backend-regex owner
  selection unless the supervisor explicitly broadens the inventory.

## Proof

No test command was run for this packet. Inventory source was the
supervisor-accepted `test_baseline.log`; `test_after.log` was read only as
context for the recently repaired representative pass states.
