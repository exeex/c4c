Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reconstruct the Post-301 Inventory

# Current Packet

## Just Finished

Completed plan Step 1: reconstructed the post-301 backend-regex residual
inventory from the accepted broad proof in `test_before.log`.

Accepted proof state:

- Selected tests: 352
- Passed tests: 300
- Failed tests: 52
- Local backend/unit failures: 0
- `c_testsuite` AArch64 backend failures: 52

Current c_testsuite failures by runner source:

- `FRONTEND_FAIL` (6): `00064`, `00139`, `00140`, `00204`, `00205`, `00216`
- `BACKEND_FAIL` (2): `00104`, `00182`
- `RUNTIME_NONZERO` (26): `00035`, `00050`, `00066`, `00086`, `00089`,
  `00102`, `00111`, `00112`, `00113`, `00119`, `00121`, `00123`, `00137`,
  `00138`, `00144`, `00151`, `00170`, `00173`, `00179`, `00181`, `00189`,
  `00200`, `00207`, `00208`, `00214`, `00215`
- `RUNTIME_MISMATCH` (15): `00157`, `00159`, `00164`, `00168`, `00169`,
  `00172`, `00174`, `00175`, `00176`, `00185`, `00186`, `00193`, `00195`,
  `00196`, `00218`
- Timeout (3): `00143`, `00187`, `00220`

Current failure list:

- `c_testsuite_aarch64_backend_src_00035_c` - `RUNTIME_NONZERO`, exit 1
- `c_testsuite_aarch64_backend_src_00050_c` - `RUNTIME_NONZERO`, exit 3
- `c_testsuite_aarch64_backend_src_00064_c` - `FRONTEND_FAIL`, printer
  cannot spell scalar `div`
- `c_testsuite_aarch64_backend_src_00066_c` - `RUNTIME_NONZERO`, exit 1
- `c_testsuite_aarch64_backend_src_00086_c` - `RUNTIME_NONZERO`, exit 1
- `c_testsuite_aarch64_backend_src_00089_c` - `RUNTIME_NONZERO`,
  segmentation fault
- `c_testsuite_aarch64_backend_src_00102_c` - `RUNTIME_NONZERO`, exit 1
- `c_testsuite_aarch64_backend_src_00104_c` - `BACKEND_FAIL`, invalid
  `sxtw w20, w13`
- `c_testsuite_aarch64_backend_src_00111_c` - `RUNTIME_NONZERO`, exit 1
- `c_testsuite_aarch64_backend_src_00112_c` - `RUNTIME_NONZERO`, exit 1
- `c_testsuite_aarch64_backend_src_00113_c` - `RUNTIME_NONZERO`, exit 1
- `c_testsuite_aarch64_backend_src_00119_c` - `RUNTIME_NONZERO`, exit 1
- `c_testsuite_aarch64_backend_src_00121_c` - `RUNTIME_NONZERO`, exit 167
- `c_testsuite_aarch64_backend_src_00123_c` - `RUNTIME_NONZERO`, exit 1
- `c_testsuite_aarch64_backend_src_00137_c` - `RUNTIME_NONZERO`, exit 1
- `c_testsuite_aarch64_backend_src_00138_c` - `RUNTIME_NONZERO`, exit 1
- `c_testsuite_aarch64_backend_src_00139_c` - `FRONTEND_FAIL`, printer
  cannot spell scalar `mul`
- `c_testsuite_aarch64_backend_src_00140_c` - `FRONTEND_FAIL`, unsupported
  call-boundary move shape
- `c_testsuite_aarch64_backend_src_00143_c` - Timeout
- `c_testsuite_aarch64_backend_src_00144_c` - `RUNTIME_NONZERO`, exit 1
- `c_testsuite_aarch64_backend_src_00151_c` - `RUNTIME_NONZERO`, exit 1
- `c_testsuite_aarch64_backend_src_00157_c` - `RUNTIME_MISMATCH`
- `c_testsuite_aarch64_backend_src_00159_c` - `RUNTIME_MISMATCH`
- `c_testsuite_aarch64_backend_src_00164_c` - `RUNTIME_MISMATCH`
- `c_testsuite_aarch64_backend_src_00168_c` - `RUNTIME_MISMATCH`
- `c_testsuite_aarch64_backend_src_00169_c` - `RUNTIME_MISMATCH`
- `c_testsuite_aarch64_backend_src_00170_c` - `RUNTIME_NONZERO`,
  segmentation fault
- `c_testsuite_aarch64_backend_src_00172_c` - `RUNTIME_MISMATCH`
- `c_testsuite_aarch64_backend_src_00173_c` - `RUNTIME_NONZERO`,
  segmentation fault
- `c_testsuite_aarch64_backend_src_00174_c` - `RUNTIME_MISMATCH`
- `c_testsuite_aarch64_backend_src_00175_c` - `RUNTIME_MISMATCH`
- `c_testsuite_aarch64_backend_src_00176_c` - `RUNTIME_MISMATCH`
- `c_testsuite_aarch64_backend_src_00179_c` - `RUNTIME_NONZERO`,
  segmentation fault
- `c_testsuite_aarch64_backend_src_00181_c` - `RUNTIME_NONZERO`,
  segmentation fault after substantial stdout
- `c_testsuite_aarch64_backend_src_00182_c` - `BACKEND_FAIL`, invalid
  immediate materialization for `mov x0, #1234567`
- `c_testsuite_aarch64_backend_src_00185_c` - `RUNTIME_MISMATCH`
- `c_testsuite_aarch64_backend_src_00186_c` - `RUNTIME_MISMATCH`
- `c_testsuite_aarch64_backend_src_00187_c` - Timeout
- `c_testsuite_aarch64_backend_src_00189_c` - `RUNTIME_NONZERO`,
  segmentation fault
- `c_testsuite_aarch64_backend_src_00193_c` - `RUNTIME_MISMATCH`
- `c_testsuite_aarch64_backend_src_00195_c` - `RUNTIME_MISMATCH`
- `c_testsuite_aarch64_backend_src_00196_c` - `RUNTIME_MISMATCH`
- `c_testsuite_aarch64_backend_src_00200_c` - `RUNTIME_NONZERO`, exit 26
  after repeated wrong shift-type output
- `c_testsuite_aarch64_backend_src_00204_c` - `FRONTEND_FAIL`, semantic
  `lir_to_bir` failed in `gep` local-memory family
- `c_testsuite_aarch64_backend_src_00205_c` - `FRONTEND_FAIL`, printer cannot
  spell scalar `logical_shift_right` unsigned reduction
- `c_testsuite_aarch64_backend_src_00207_c` - `RUNTIME_NONZERO`,
  segmentation fault
- `c_testsuite_aarch64_backend_src_00208_c` - `RUNTIME_NONZERO`,
  segmentation fault
- `c_testsuite_aarch64_backend_src_00214_c` - `RUNTIME_NONZERO`,
  segmentation fault
- `c_testsuite_aarch64_backend_src_00215_c` - `RUNTIME_NONZERO`,
  segmentation fault
- `c_testsuite_aarch64_backend_src_00216_c` - `FRONTEND_FAIL`, semantic
  `lir_to_bir` failed in load local-memory family
- `c_testsuite_aarch64_backend_src_00218_c` - `RUNTIME_MISMATCH`, unsigned
  enum bit-field output
- `c_testsuite_aarch64_backend_src_00220_c` - Timeout

Likely Step 2 residual family buckets from the log and test source headers:

- Printer/selector shape gaps: scalar `mul`/`div`, unsigned reduction
  `logical_shift_right`, and call-boundary move selection.
- AArch64 assembly legality gaps: sign-extension register-width selection and
  large immediate materialization.
- Semantic admission/local-memory gaps: `gep` and load local-memory failures
  still block `00204` and `00216` before prepared-module handoff.
- Runtime scalar correctness gaps: unary/logical operators, promotions,
  short/char sign or zero extension, shifts, integer-to-float conversion, and
  enum/bit-field extension.
- Runtime aggregate, pointer, and local/global memory gaps: structs, unions,
  arrays, initializer shapes, address/pointer flow, string buffers, and
  function-pointer paths.
- Runtime call/control-flow gaps: recursion, nested loops, switch/return
  control, indirect calls, variadic/ABI-adjacent calls, and larger program
  state machines.
- Timeout/output-storm bucket: `00143`, `00187`, and `00220` need to remain
  separate until hang versus output volume versus runtime library behavior is
  proven.

## Suggested Next

Execute plan Step 2 by turning the Step 1 inventory into a small number of
owned residual families, using generated-code or diagnostic evidence before
choosing any implementation owner.

## Watchouts

- This umbrella plan is classification-only; switch to a focused owner before
  implementation.
- Do not change expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, CTest registration, or test contracts.
- Do not reopen ideas 285 through 301 from counts alone; require
  generated-code, diagnostic, or proof evidence that contradicts closure
  boundaries.
- Keep timeout or output-storm cases separate unless evidence supports a
  hang-specific owner.
- The broad proof shows all 52 residual failures are `c_testsuite` AArch64
  backend failures; local backend/unit tests selected by the broad regex passed.
- `test_before.log` contains a very large output region before the final CTest
  summary; avoid rerunning broad runtime tests unless the supervisor explicitly
  requests a fresh proof.

## Proof

Used existing accepted broad proof `test_before.log`; no tests were rerun per
packet instruction. The accepted proof records 352 selected, 300 passed, and
52 failed after idea 301.
