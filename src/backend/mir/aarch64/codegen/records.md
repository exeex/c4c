# AArch64 Target Record And Machine-Node Core

The AArch64 codegen record core has two active structured layers.
Target-MIR records preserve prepared register, value, frame, symbol, block,
and source BIR facts before instruction selection. Selected machine-node
records are the downstream structured instruction-like layer produced from
the accepted target-MIR subset. Neither layer is assembly text, parsed
assembly, object bytes, or linker input.

Deferred behavior guardrails:

- Branch records carry block labels and optional condition operands only; branch
  lowering, layout decisions, and relocation choices are deferred.
- Branch/compare target-MIR records preserve `BlockLabelId`,
  `PreparedValueId`, `ValueNameId`, BIR predicate, type, operand, and
  fusion-candidate metadata. Selected branch machine-node records may carry
  structured branch/conditional-branch/compare-branch opcode identity for the
  accepted subset, but they still do not print `cmp`, `cset`, `b.cond`,
  `cbz`, `cbnz`, `tbz`, `tbnz`, or any other assembly syntax.
- Branch target pairs and compare candidates are structured ids plus source
  facts only. Assembly mnemonics, condition-code spelling, branch relaxation,
  encoding width, relocation records, object writing, and linker behavior are
  outside this layer.
- Scalar target-MIR records may keep source BIR opcode metadata; selected
  scalar machine-node records add structured `MachineOpcode` identity for the
  accepted integer ALU and cast subset while leaving flag-setting details,
  immediate encoding legality, printing, and object encoding to later
  consumers.
- Scalar ALU records currently support only integer `Add`, `Sub`, `And`, `Or`,
  and `Xor` as explicit `ScalarAluOperationKind` values. They preserve the
  source BIR binary opcode, operand type, result type, result `PreparedValueId`,
  result `ValueNameId`, and structured `OperandRecord` inputs before selection.
  Selected machine-node records for this subset carry typed operands, def/use
  resources, and `MachineOpcode::Add`, `Sub`, `And`, `Or`, or `Xor`.
- Scalar cast records currently support only simple integer `SExt`, `ZExt`,
  and `Trunc` as explicit `ScalarCastOperationKind` values. They preserve the
  source BIR cast opcode, source type, result type, result `PreparedValueId`,
  result `ValueNameId`, and structured source `OperandRecord` before selection.
  Selected machine-node records for this subset carry typed operands, def/use
  resources, and `MachineOpcode::SignExtend`, `ZeroExtend`, or `Truncate`.
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
- Allocation-result records are the only target MIR authority for long-lived
  physical homes, structured spill-slot ids, reserved MIR scratch use, call
  preservation resources, and future virtual-register placeholders. Scalar,
  branch, memory, call, return, vector, inline-asm, and prologue records may
  consume that data, but must not recover it from rendered register names or
  create local allocation policy.
- Move, ABI-binding, parallel-copy, spill, and reload consumers should use
  `module::MoveRecord`, `module::AbiBindingRecord`,
  `module::ParallelCopyRecord`, and `module::SpillReloadRecord` as their
  allocation-sensitive authority. Missing prepared provenance, destination
  slot identity, or carrier details are separate prepared-carrier idea
  candidates, not permission for codegen records to infer from raw BIR,
  rendered names, or assembly text.
- Memory target-MIR records carry prepared address facts and base/offset shape
  hints. Selected memory machine-node records exist for the accepted
  prepared-memory subset and carry structured load/store opcode identity,
  typed operands, def/use resources, and memory side effects; final addressing
  legality, register-width spelling, writeback forms, printing, and object
  encoding remain deferred.
- Memory operand records support prepared frame-slot, global-symbol,
  pointer-value, and string-constant bases when structured prepared/BIR facts
  provide matching identity. They preserve function, block, instruction,
  result/stored value ids, base ids, byte offset, size, alignment,
  volatility, address space, and base-plus-offset eligibility as data.
- Prepared pointer-value memory records must join
  `PreparedAddress::pointer_value_name` to a unique prepared value-location
  home before recording `PreparedValueId`. Missing, mismatched, or ambiguous
  pointer homes fail closed instead of falling back to rendered names.
- Prepared string-constant memory records preserve structured string symbol
  identity through `LinkNameId` and preserve `TextId` when the prepared text
  table provides it. They do not parse labels from formatted assembly or BIR
  dumps.
- Volatility and address space are upstream facts. Direct records preserve
  `is_volatile` and `bir::AddressSpace` exactly, including non-default address
  spaces. Prepared conversion rejects mismatched BIR/prepared volatility,
  address-space, offset, size, or alignment facts, and rejects missing
  structured BIR address facts when prepared facts require them.
- Unsupported memory bases and incomplete prepared facts use explicit
  `PreparedMemoryOperandRecordError` results or `DeferredUnsupported` record
  vocabulary. This layer must fail closed instead of inventing target-local
  defaults that hide upstream facts.
- Memory instruction records wrap memory operand data as structured load/store
  intent. Selected memory machine-node records may choose `MachineOpcode::Load`
  or `MachineOpcode::Store`, but they do not select `ldr`, `str`, register
  width spelling, final addressing mode, writeback form, scratch registers,
  assembly text, object encoding, relocation records, calls, or returns.
- Call records carry callee, argument, result, memory-return, preserved-value,
  clobber, and calling convention metadata. AAPCS64 call-boundary authority is
  `../AAPCS64_CALL_RETURN_FRAME_CONTRACT.md`; final call sequence emission,
  relocation/object behavior, and deferred carriers such as total outgoing
  call-area or direct variadic-count fields remain later work.
- Return records carry optional return values only. Before-return movement,
  ABI-binding facts, link-register treatment, and epilogue boundaries are
  governed by `../AAPCS64_CALL_RETURN_FRAME_CONTRACT.md`; final return
  instruction emission and a dedicated return-boundary record are deferred.
- Spill/reload target-MIR records remain `module::SpillReloadRecord`
  snapshots. Selected spill/reload pseudo-node records are derived only from
  prepared spill/reload target-MIR records and preserve value identity, scratch
  authority, occupied scratch registers, spill-slot id, memory operand facts,
  opcode/pseudo identity, def/use resources, and memory side effects. They are
  not stack-address text, local scratch allocation, rematerialization, or final
  load/store assembly.
- Assembler records distinguish external assembler input from internal
  selected nodes. External assembler records may carry operands and
  side-effect flags for a future assembler contract, but assembly text,
  inline-asm parsing, and textual emission are not internal codegen handoffs.
- Object records carry symbol, frame-slot, value, and type facts for future
  structured consumers only; object encoding, relocation writing, and linker
  behavior are deferred.

Current records are classified by their explicit surface kind. Target MIR and
pre-node records remain the prepared-fact snapshot layer. Selected branch,
scalar, memory, and spill/reload records use the machine-instruction-node
surface. Printer-output, encoder-input, object, and external-assembler-input
surfaces are downstream consumers or future contracts, not sources for
codegen-owned semantics. New fields here should keep prepared identities
structured and typed instead of parsing rendered names or growing
`module/module.cpp`. `RecordSurfaceKind::RecordOnly` is only a compatibility
spelling for the target-MIR/pre-node surface; new roadmap work should prefer
explicit target-MIR, selected machine-node, printer-output, encoder-input, or
external-assembler-input names.
