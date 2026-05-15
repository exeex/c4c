Status: Active
Source Idea Path: ideas/open/235_aarch64_scalar_cast_and_float_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Print FP Arithmetic And Conversion Nodes

# Current Packet

## Just Finished

Step 6 printed selected F32/F64 arithmetic and supported conversion nodes from
structured register/type/bank-transition fields.

Completed work:

- `print_scalar` now prints selected F32/F64 scalar arithmetic records from
  structured FP/SIMD destination/source operands.
- F32/F64 `Add`, `Sub`, `Mul`, and `Div` print as `fadd`, `fsub`, `fmul`, and
  `fdiv` using S/D register views derived from record type facts.
- Supported `SIToFP`, `UIToFP`, `FPToSI`, `FPToUI`, `FPExt`, and `FPTrunc`
  print as `scvtf`, `ucvtf`, `fcvtzs`, `fcvtzu`, and `fcvt` from structured
  GPR/FPR source/result register facts.
- Printer validation uses record type widths and bank-specific register
  conversion helpers; it does not infer FP behavior from rendered names or
  route F32/F64 through W/X raw-bit shortcuts.
- Unsupported F128/binary128, pointer/bitcast, incomplete source/register
  facts, and non-FP operands still fail closed with printer diagnostics.
- Focused printer tests cover F32/F64 arithmetic output and all supported
  conversion output forms.

## Suggested Next

Step 7 validation packet: run the active focused or broader backend proof and
record semantic coverage for integer casts, F32/F64 arithmetic, typed
conversions, fail-closed F128/pointer/bitcast behavior, and no raw-name
inference.

## Watchouts

- Simple integer cast printing exists, but pointer and bitcast casts remain
  intentionally unsupported.
- Do not route F32/F64 through GPR raw-bit conventions or revive the archived
  accumulator/scratch approach.
- Keep the integer-only `scalar_register_view` behavior intact; use the
  adjacent F32/F64 view path for FP/SIMD register facts.
- F128/binary128 must remain rejected or delegated to the later soft-float
  route; do not lower F128 through scalar F64 paths.

## Proof

Fresh focused backend proof passed and wrote `test_after.log`:

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_scalar_alu_records|backend_aarch64_prepared_scalar_alu_records|backend_aarch64_scalar_cast_records|backend_aarch64_prepared_scalar_cast_records|backend_aarch64_instruction_dispatch|backend_aarch64_scalar_record_contract|backend_aarch64_machine_printer)$') > test_after.log 2>&1`

Result: 7/7 focused backend MIR tests passed.
