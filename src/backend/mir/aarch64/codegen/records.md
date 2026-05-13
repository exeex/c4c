# AArch64 Target Record Core

The AArch64 target record core is a typed data surface. It preserves prepared
register, value, frame, symbol, block, and source BIR facts for later lowering
without choosing concrete AArch64 instructions or formatting output.

Deferred behavior guardrails:

- Branch records carry block labels and optional condition operands only; branch
  lowering, layout decisions, and relocation choices are deferred.
- Branch/compare records preserve `BlockLabelId`, `PreparedValueId`,
  `ValueNameId`, BIR predicate, type, operand, and fusion-candidate metadata.
  They distinguish materialized boolean conditions, fused compare-and-branch
  candidates, and non-fusable compare facts without choosing `cmp`, `cset`,
  `b.cond`, `cbz`, `cbnz`, `tbz`, `tbnz`, or any other concrete opcode.
- Branch target pairs and compare candidates are structured ids plus source
  facts only. Assembly mnemonics, condition-code spelling, branch relaxation,
  encoding width, relocation records, object writing, and linker behavior are
  outside this layer.
- Scalar records may keep source BIR opcode metadata; concrete AArch64 opcode
  selection and flag behavior are deferred.
- Scalar ALU records currently support only integer `Add`, `Sub`, `And`, `Or`,
  and `Xor` as explicit `ScalarAluOperationKind` values. They preserve the
  source BIR binary opcode, operand type, result type, result `PreparedValueId`,
  result `ValueNameId`, and structured `OperandRecord` inputs.
- Scalar cast records currently support only simple integer `SExt`, `ZExt`,
  and `Trunc` as explicit `ScalarCastOperationKind` values. They preserve the
  source BIR cast opcode, source type, result type, result `PreparedValueId`,
  result `ValueNameId`, and structured source `OperandRecord`.
- Unsupported scalar ALU opcodes such as multiply, shifts, division/remainder,
  and compare predicates map to explicit `Deferred` vocabulary or fail closed
  during prepared conversion. Unsupported scalar casts such as floating-point
  casts, pointer/integer casts, bitcasts, `i128`, `f128`, and broad floating
  forms likewise defer or fail closed instead of becoming generic placeholder
  instructions.
- Prepared scalar ALU/cast conversion consumes structured BIR instructions plus
  prepared value-location and storage-plan facts. It must not recover operands
  from rendered value names or text dumps, and missing or inconsistent prepared
  homes/storage facts are errors rather than fallback selection.
- Scalar records do not own concrete AArch64 mnemonics, condition-code spelling,
  flag-setting behavior, immediate encoding legality, register allocation,
  assembly text, object encoding, memory lowering, calls, or returns.
- Memory records carry prepared address facts and base/offset shape hints only;
  load/store instruction choice, addressing legality, and spill code are
  deferred.
- Call records carry callee, argument, result, and calling convention metadata;
  AAPCS64 argument assignment, call sequence emission, and variadic policy are
  deferred.
- Return records carry optional return values only; epilogue construction and
  return instruction emission are deferred.
- Assembler records are placeholders for operands and side-effect flags only;
  assembly text, inline-asm parsing, and textual emission are deferred.
- Object records carry symbol, frame-slot, value, and type facts only; object
  encoding, relocation writing, and linker behavior are deferred.

`RecordSurfaceKind::RecordOnly` is the contract for this layer. New fields here
should keep prepared identities structured and typed instead of parsing rendered
names or growing `module/module.cpp`.
