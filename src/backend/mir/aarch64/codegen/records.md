# AArch64 Target Record And Machine-Node Core

The AArch64 codegen record core has two active structured layers.
Target-MIR records preserve prepared register, value, frame, symbol, block,
and source BIR facts before instruction selection. Selected machine-node
records are the downstream structured instruction-like layer produced from
the accepted target-MIR subset. Neither layer is assembly text, parsed
assembly, object bytes, or linker input.

Deferred behavior guardrails:

- Branch records carry block labels and optional condition operands only; branch
  lowering, layout decisions, and relocation choices remain outside the record
  layer. The current `.s` printer may print selected branch nodes in the
  supported subset, but printer syntax is not record identity.
- Branch/compare target-MIR records preserve `BlockLabelId`,
  `PreparedValueId`, `ValueNameId`, BIR predicate, type, operand, and
  fusion-candidate metadata. Selected branch machine-node records may carry
  structured branch/conditional-branch/compare-branch opcode identity for the
  accepted subset. Final syntax such as `cmp`, `cset`, `b.cond`, `cbz`,
  `cbnz`, `tbz`, or `tbnz` belongs to the `.s` printer or lower encoding
  records, not to parser-recovered semantic state.
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
  or `MachineOpcode::Store`. The current printer can print only the supported
  selected store/spill/reload address forms and fails closed otherwise; register
  width spelling, broader addressing modes, writeback forms, scratch registers,
  object encoding, relocation records, calls, and returns remain outside this
  record layer.
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
codegen-owned semantics. The public `.s` route prints from selected machine
nodes; it does not route that assembly through the in-tree parser, encoder,
object writer, or linker. New fields here should keep prepared identities
structured and typed instead of parsing rendered names or growing
`module/module.cpp`. `RecordSurfaceKind::RecordOnly` is only a compatibility
spelling for the target-MIR/pre-node surface; new roadmap work should prefer
explicit target-MIR, selected machine-node, printer-output, encoder-input, or
external-assembler-input names.

Minimum future structured asm/encoding records are a downstream stream derived
from selected machine instruction nodes. The accepted families are a
translation-unit record, section record, label record,
operator/instruction record, directive record, data record, typed operand
records, and a `RelocationNeed`-style relocation-need record. These records
preserve semantic payload for object and linker work: typed operands,
register/immediate facts, symbol ids, `LinkNameId`, relocation kind/addend,
section and data ownership, branch and data references, volatility,
address-space facts, side effects, clobbers, and provenance back to prepared
values, frame slots, allocation facts, and selected machine nodes.

Register operands in that structured asm/encoding layer must stay decomposed
instead of becoming one broad register-shaped payload. The contract names the
minimum pieces as `AsmRegisterRef`, `AsmRegisterUseKind`, `AsmRegisterUse`,
`AsmValueProvenance`, `AsmAllocationProvenance`, and `AsmRegisterOperand`.
`AsmRegisterRef` owns the physical AArch64 register reference and view.
`AsmRegisterUseKind` and `AsmRegisterUse` describe input, output, inout,
scratch, clobber, address-base, call-argument, return-value, and implicit-use
roles. `AsmValueProvenance` owns prepared value id, value name, and type when
the register carries a prepared value. `AsmAllocationProvenance` owns prepared
register class or bank, contiguous width, and occupied registers when the
operand came from allocation. Clobber and side-effect metadata remains
separate on the operator/instruction or effect record so register reference,
register use, value provenance, allocation provenance, and clobber/effect
metadata do not collapse into a catch-all operand.

Structured asm/encoding records are downstream of the selected machine-node
surface and downstream of prepared authority. They may reference
`PreparedLiveness`, `PreparedLiveInterval`, `PreparedRegalloc`,
`PreparedRegallocValue`, `PreparedInterferenceEdge`,
`PreparedMoveResolution`, `PreparedSpillReloadOp`,
`PreparedValueLocations`, `PreparedCallPreservedValue`, and
`PreparedClobberedRegister` as provenance for value homes, interference,
move resolution, spill/reload records, call-preserved values, and clobbered
registers. They must not invent local allocation, liveness, preservation, or
spill policy.

The additional information owned by the selected machine-node or lower
encoding surface is the post-selection machine effect set: implicit register
uses/defs, selected opcode clobber facts, flags, target scratch lifetimes,
operator side effects, final section placement, and section/relocation
ownership for object-facing records. That effect set remains attached to
machine instruction nodes or derived structured records, preserving machine
instruction nodes as the semantic boundary before any printer, encoder,
object writer, linker, or external assembler parser consumes the stream.

Future record surfaces must keep enum spelling centralized. Any new section
enum, label enum, directive enum, operator/opcode enum, operand kind enum,
register use kind enum such as `AsmRegisterUseKind`, relocation kind enum, or
record surface enum needs one enum-to-string or to_string mapping family for
terminal printers and diagnostics. Lowering and record construction should
consume typed enum values; terminal printers and diagnostics should call the
central mapping instead of scattering display switches through semantic
lowering code. No live helper is needed for this docs-only plan because it has
not introduced new live structured asm/encoding enum types.
