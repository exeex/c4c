Status: Active
Source Idea Path: ideas/open/430_rv64_integer_div_rem_lowering.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Lower Or Validate Scalar Remainder

# Current Packet

## Just Finished

Completed Step 2, Lower Or Validate Scalar Division.

Implemented the focused validation packet for scalar division in
`tests/backend/mir/backend_riscv_object_emission_test.cpp`; implementation
files did not need changes.

Coverage added:

- `UDiv i32` now has focused object-emission coverage for RV64 `divuw`.
- `UDiv i64` now has focused object-emission coverage for RV64 `divu`.
- Existing `SDiv i32`/`SDiv i64` focused coverage remains in the same test.

Fail-closed coverage added:

- missing RHS prepared home rejects with `unsupported_instruction_fragment`;
- missing result prepared home rejects with `unsupported_instruction_fragment`;
- pointer-typed `UDiv` rejects with `unsupported_instruction_fragment`.

Step artifact: `build/agent_state/430_step2_scalar_division/summary.tsv`.
The representative rows were not reclassified as passing in this packet; the
failing rows from Step 1 still include call, select, frame-slot argument,
local-publication, or global/select residuals.

## Suggested Next

Execute Step 3: validate scalar remainder lowering by preserving focused
`SRem`/`URem` coverage for `i32`/`i64`, tightening fail-closed remainder shapes
if needed, and avoiding representative-row claims.

## Watchouts

- Do not use divisor-specific handling, representative filenames, or row-shaped
  shortcuts.
- Preserve signedness: `sdiv`/`srem` must not route through unsigned behavior,
  and `udiv`/`urem` must not route through signed behavior.
- Do not claim row-level representative progress from focused div/rem coverage;
  the failing representatives also contain call, select, frame-slot argument,
  local-publication, or global/select residuals.
- Keep the Step 2 implementation boundary: no object-emission code changes
  were required for division.
- Keep floating-point division, F128/I128 helper work, pointer/address,
  select/join, aggregate ABI, call publication, and global memory residuals
  outside this plan.
- Do not change expectations, allowlists, unsupported markers, runtime
  comparison, or pass/fail accounting.

## Proof

Step 2 focused division validation proof passed:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Proof log: `test_after.log`.
