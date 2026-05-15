Status: Active
Source Idea Path: ideas/open/241_f128_full_width_constant_carriers.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Expose Constant Carriers To AArch64 Selection

# Current Packet

## Just Finished

Step 4 F128 constant-carrier selection exposure is complete. AArch64
call-boundary selection now accepts F128 immediate argument moves only when the
prepared argument source id resolves to a structured full-width carrier with a
matching low/high `constant_payload`, and the selected record keeps that
payload attached without using F64, rendered text, or a single 64-bit lane.
Dispatch tests cover the valid carrier plus missing payload, missing source id,
and scalar-only literal rejection paths.

## Suggested Next

Delegate Step 5 to prove the dependency end to end and guard adjacent scalar,
i128, helper, and printer routes without claiming constant assembly printing
beyond the selected structured record.

## Watchouts

- F128 immediate carriers remain `kind=missing`; selection treats that as valid
  only for the constant-carrier case when `constant_payload` is complete and
  tied to the prepared source value id.
- The selected call-boundary record exposes payload authority but does not add
  AArch64 constant materialization printing. Keep printer support gated on the
  same complete structured facts.
- V-register immediate call arguments without a complete F128 payload now
  diagnose as missing value authority instead of falling through to scalar or
  generic register-move routes.

## Proof

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_prepare_liveness|backend_prepared_printer|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|backend_aarch64_prepared_scalar_alu_records|backend_aarch64_scalar_alu_records|backend_aarch64_prepared_scalar_cast_records|backend_aarch64_scalar_cast_records)$'; } 2>&1 | tee test_after.log`

Result: passed. Proof log: `test_after.log`.
