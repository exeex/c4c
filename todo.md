Status: Active
Source Idea Path: ideas/open/235_aarch64_scalar_cast_and_float_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Print Simple Integer Cast Nodes

# Current Packet

## Just Finished

Step 3 printed selected simple integer `bir::CastInst` nodes from structured
machine-record facts.

Completed work:

- `print_scalar` now recognizes selected `ScalarCastRecord` nodes before the
  add/sub path and prints only the simple integer cast subset.
- `SExt` prints typed AArch64 sign-extension forms from source/result widths,
  including `sxtw` for I32-to-I64 and an explicit `sbfx` form for I1.
- `ZExt` prints explicit low-bit `ubfx` forms, preserving the structured
  destination/source physical register identities while choosing the required
  W/X textual views.
- `Trunc` prints low-view `mov` for I64-to-I32 and masks narrower integer
  truncations with structured register operands.
- Missing cast source registers, non-GPR cast operands, unsupported integer
  widths, and FP/pointer/bitcast/F128-shaped selected records remain
  fail-closed with printer diagnostics.
- Focused printer tests cover `SExt`, `ZExt`, and `Trunc` terminal output plus
  malformed selected cast diagnostics. No FP cast or FP arithmetic output was
  added.

## Suggested Next

Step 4 implementation packet: inspect/select the narrow F32/F64 scalar FP
operation subset with explicit FP/SIMD record fields and prepared FPR storage
authority, without adding F128 or implicit GPR raw-bit conventions.

## Watchouts

- Simple integer cast printing now exists, but pointer, bitcast, float/int, and
  FP width conversion casts remain intentionally unsupported.
- Do not route F32/F64 through GPR raw-bit conventions or revive the archived
  accumulator/scratch approach.
- F32/F64 work needs explicit FP/SIMD record fields and S/D register views, not
  changes that make `scalar_register_view` treat floating types as W/X.
- F128/binary128 must remain rejected or delegated to the later soft-float
  route; do not lower F128 through scalar F64 paths.
- `MachineOpcode::SignExtend`, `ZeroExtend`, and `Truncate` still have no
  generic mnemonic-kind mapping; the printer handles them through the structured
  `ScalarCastRecord` path because width/sign determines the real AArch64 form.

## Proof

Fresh focused backend proof passed and wrote `test_after.log`:

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_prepared_scalar_cast_records|backend_aarch64_instruction_dispatch|backend_aarch64_scalar_record_contract|backend_aarch64_machine_printer)$') > test_after.log 2>&1`

Result: 4/4 focused backend MIR tests passed.
