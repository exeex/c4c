Status: Active
Source Idea Path: ideas/open/430_rv64_integer_div_rem_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Re-Probe Representatives And Decide Close Readiness

# Current Packet

## Just Finished

Completed Step 3, Lower Or Validate Scalar Remainder.

Implemented the focused validation packet for scalar remainder in
`tests/backend/mir/backend_riscv_object_emission_test.cpp`; implementation
files did not need changes.

Coverage preserved and validated:

- `SRem i32` has focused object-emission coverage for RV64 `remw`.
- `SRem i64` has focused object-emission coverage for RV64 `rem`.
- `URem i32` has focused object-emission coverage for RV64 `remuw`.
- `URem i64` has focused object-emission coverage for RV64 `remu`.

Fail-closed coverage added:

- missing LHS prepared home for `SRem` rejects with
  `unsupported_instruction_fragment`;
- missing result prepared home for `URem` rejects with
  `unsupported_instruction_fragment`;
- pointer-typed `SRem` rejects with `unsupported_instruction_fragment`.

Step artifact: `build/agent_state/430_step3_scalar_remainder/summary.tsv`.
The representative rows were not reclassified as passing in this packet; the
failing rows from Step 1 still include call, select, frame-slot argument,
local-publication, or global/select residuals.

## Suggested Next

Execute Step 4: re-probe the representative div/rem rows and decide close
readiness for this source idea, separating focused div/rem coverage from
remaining call/select/local-publication/global residuals.

## Watchouts

- Do not use divisor-specific handling, representative filenames, or row-shaped
  shortcuts.
- Preserve signedness: `sdiv`/`srem` must not route through unsigned behavior,
  and `udiv`/`urem` must not route through signed behavior.
- Do not claim row-level representative progress from focused div/rem coverage;
  the failing representatives also contain call, select, frame-slot argument,
  local-publication, or global/select residuals.
- Keep the Step 2/Step 3 implementation boundary: no object-emission code
  changes were required for division or remainder.
- Keep floating-point division, F128/I128 helper work, pointer/address,
  select/join, aggregate ABI, call publication, and global memory residuals
  outside this plan.
- Do not change expectations, allowlists, unsupported markers, runtime
  comparison, or pass/fail accounting.

## Proof

Step 3 focused remainder validation proof passed:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Proof log: `test_after.log`.
