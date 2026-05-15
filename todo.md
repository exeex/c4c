Status: Active
Source Idea Path: ideas/open/241_f128_full_width_constant_carriers.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Transport F128 Constants Through Prepared State

# Current Packet

## Just Finished

Step 3 F128 constant transport is complete. Regalloc now gives real BIR F128
immediate literals synthetic prepared value identity, value homes and storage
plans carry the full low/high payload as `immediate_f128`, call plans attach
the prepared source value id to immediate F128 arguments, and F128 carriers
publish the same full-width payload from prepared storage identity. The tests
cover the real prepared route, existing scalar immediate/i128 behavior, and the
AArch64 missing-carrier diagnostic without adding AArch64 constant selection.

## Suggested Next

Delegate Step 4 to add the next bounded backend-facing behavior for F128
constant carriers, keeping AArch64 constant selection separate from unrelated
scalar immediate and helper routes.

## Watchouts

- F128 immediate carriers are intentionally still `kind=missing` with a
  complete `constant_payload`; this exposes payload authority by prepared value
  id without teaching AArch64 to select or print the constant yet.
- Existing scalar immediates still use `immediate_i32`; do not route F128
  constants through that field, F64, rendered text, or a single 64-bit lane.
- The synthetic prepared constant name is stable payload-derived spelling; the
  authority remains the prepared value id plus `constant_payload`.

## Proof

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_prepare_liveness|backend_prepared_printer|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|backend_aarch64_prepared_scalar_alu_records|backend_aarch64_scalar_alu_records|backend_aarch64_prepared_scalar_cast_records|backend_aarch64_scalar_cast_records)$'; } 2>&1 | tee test_after.log`

Result: passed. Proof log: `test_after.log`.
