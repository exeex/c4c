Status: Active
Source Idea Path: ideas/open/241_f128_full_width_constant_carriers.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Current Constant Payload Authority

# Current Packet

## Just Finished

Step 1 inventory and fail-closed tests are complete. Current authority is still
limited to integer immediates plus F32/F64 `immediate_bits`; there is no BIR
F128 immediate factory or full-width constant carrier. LIR scalar F128
constants fail in the scalar-binop path, and prepared F128 helper publication
rejects manually constructed immediate operands because helper authority
requires named full-width carrier operands.

## Suggested Next

Delegate the next packet to design the minimal full-width F128 constant carrier
shape and the first carrier-preserving lowering boundary, without routing it
through F64, `immediate_bits`, rendered text, or a single 64-bit lane.

## Watchouts

- Do not claim F128 constant progress through `F64`, `immediate`,
  `immediate_bits`, `double`, or any single-lane 64-bit payload.
- Existing x86_fp80 aggregate/global paths can lower byte elements, but they are
  not scalar F128 constant authority.
- Do not add AArch64 constant assembly printing before a structured full-width
  constant carrier exists and reaches selection.
- Keep arithmetic, comparison, cast, helper-call, atomic, intrinsic, inline
  assembly, scalar FP, and i128 routes out of this plan unless the source idea
  explicitly requires a shared carrier-preserving touch.

## Proof

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_prepare_liveness|backend_prepared_printer|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|backend_aarch64_prepared_scalar_alu_records|backend_aarch64_scalar_alu_records|backend_aarch64_prepared_scalar_cast_records|backend_aarch64_scalar_cast_records)$'; } 2>&1 | tee test_after.log`

Result: passed. Proof log: `test_after.log`.
