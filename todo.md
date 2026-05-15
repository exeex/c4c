Status: Active
Source Idea Path: ideas/open/235_aarch64_scalar_cast_and_float_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Scalar Cast And FP Surfaces

# Current Packet

## Just Finished

Step 1 inspected the current AArch64 scalar cast and F32/F64 operation
surfaces.

Current carriers:

- `bir::CastInst` carries `opcode`, `result`, and `operand`. The opcode
  vocabulary already includes `SExt`, `ZExt`, `Trunc`, `FPTrunc`, `FPExt`,
  `FPToSI`, `FPToUI`, `SIToFP`, `UIToFP`, `PtrToInt`, `IntToPtr`, and
  `Bitcast`.
- `ScalarCastRecord` preserves source opcode, source/result types, prepared
  result id/name, source operand, and a `supported_simple_integer_cast` flag.
  It does not currently carry a structured destination register.
- `ScalarInstructionRecord` has a `result_register`, but
  `make_scalar_cast_instruction_record` does not populate it from
  `ScalarCastRecord`, so selected cast records cannot yet print or feed later
  scalar state as structured register defs.
- `ScalarAluRecord` already carries a structured result register and structured
  `lhs`/`rhs` operands for selected integer ALU operations.
- Prepared value-home/storage facts are available through
  `PreparedValueLocationFunction` and `PreparedStoragePlanFunction`.
  `PreparedRegisterBank` already distinguishes `Gpr`, `Fpr`, `Vreg`, and
  `AggregateAddress`, and AArch64 ABI conversion can convert representative
  `sN`/`dN` FP/SIMD spellings and structured FPR placements.

Current simple-cast path:

- `make_prepared_scalar_cast_record` supports only `SExt`, `ZExt`, and
  `Trunc`. It validates named result homes, source operands, result storage,
  and register spellings/placements, then returns a selected-style record.
- Unsupported cast opcodes such as `FPExt`, `FPTrunc`, float/int conversions,
  pointer casts, and bitcasts return `UnsupportedOpcode`.
- Unsupported operand/result types return `UnsupportedOperandType`; missing
  homes/storage and register view mismatches already fail closed through
  explicit `PreparedScalarCastRecordError` values.
- `dispatch_prepared_block` reaches `lower_scalar_instruction`, but
  `lower_scalar_instruction` currently only handles `bir::BinaryInst`.
  `bir::CastInst` still falls through to unsupported-instruction diagnostics.

Current ALU/FP path:

- `make_prepared_scalar_alu_record` supports only integer `Add`, `Sub`, `And`,
  `Or`, and `Xor` through `is_scalar_alu_integer_opcode`.
- `scalar_register_view` maps integer and pointer types to W/X views and
  explicitly returns `std::nullopt` for `F32`, `F64`, and `F128`.
- As a result, F32/F64 arithmetic cannot yet form scalar ALU records even when
  prepared FPR/SIMD homes and storage facts exist.
- `F128` is also rejected by the same type gate; this matches the source idea's
  requirement to leave binary128/F128 to a separate soft-float route rather
  than treating it as F64.

Printer paths and diagnostics:

- `print_scalar` currently prints only selected `Add` and `Sub` machine opcodes
  from structured destination/source operands. It rejects other scalar opcodes
  with `scalar node opcode is outside the printable add/sub subset`.
- `MachineOpcode::SignExtend`, `ZeroExtend`, and `Truncate` intentionally have
  no current printer mnemonic mapping, so even manually selected cast nodes are
  not printable yet.
- Existing scalar tests cover explicit deferred vocabulary for unsupported
  cast and ALU forms, prepared scalar ALU/cast record validation, storage
  placement preference, and fail-closed register conversion cases.

First implementation target:

- Execution can proceed to Step 2 without a separate prepared/register-bank
  blocker, but Step 2 should stay limited to simple integer casts.
- The first implementation packet should add structured destination-register
  authority to the simple integer cast machine record path, consume
  `bir::CastInst` in AArch64 scalar dispatch, record emitted cast result
  registers for later scalar/return consumers, and keep FP casts, pointer casts,
  bitcasts, and F128 cases fail-closed.

## Suggested Next

Step 2 implementation packet: select simple integer cast nodes.

Suggested packet:

`to_subagent: c4c-executor`
`Objective: Execute Step 2 by lowering prepared simple integer bir::CastInst SExt/ZExt/Trunc operations into selected AArch64 scalar machine records with structured destination and source register facts, without terminal printer output.`
`Plan Step: Step 2: Select Simple Integer Cast Nodes`
`Owned Files: todo.md, src/backend/mir/aarch64/codegen/instruction.hpp, src/backend/mir/aarch64/codegen/instruction.cpp, src/backend/mir/aarch64/codegen/alu.cpp, tests/backend/mir/backend_aarch64_prepared_scalar_cast_records_test.cpp, tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp, tests/backend/mir/backend_aarch64_scalar_record_contract_test.cpp if needed`
`Do Not Touch: plan.md, ideas/open/235_aarch64_scalar_cast_and_float_machine_nodes.md, terminal printer output paths, unrelated frontend/x86 files`
`Proof: (cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_prepared_scalar_cast_records|backend_aarch64_instruction_dispatch|backend_aarch64_scalar_record_contract)$') > test_after.log 2>&1`
`Done When: supported SExt/ZExt/Trunc CastInst values select from prepared value-home/storage facts into structured scalar machine records carrying destination register, source operand, source/result type and opcode facts; dispatch records emitted cast registers; unsupported FP/pointer/bitcast/F128 cases and missing register facts diagnose or defer explicitly. Do not add printer output in this packet.`

## Watchouts

- Do not add pointer, bitcast, float/int, or FP width conversion semantics in
  the simple integer cast packet.
- Do not route F32/F64 through GPR raw-bit conventions or revive the archived
  accumulator/scratch approach.
- F32/F64 work needs explicit FP/SIMD record fields and S/D register views, not
  changes that make `scalar_register_view` treat floating types as W/X.
- F128/binary128 must remain rejected or delegated to the later soft-float
  route; do not lower F128 through scalar F64 paths.
- Printer work for casts belongs to Step 3.

## Proof

Inspection-only packet. No build/tests were run, and `test_after.log` was not
created or modified.
