# AArch64 Machine Instruction Node Contract

This contract defines the structured AArch64 machine-instruction-node boundary
between target MIR records and assembler, printer, encoder, and object layers.
It is the authority for later AArch64 codegen work that needs a downstream
instruction representation.

## Pipeline Position

The accepted AArch64 lowering pipeline is:

```text
PreparedBirModule
  -> AArch64 target MIR records
  -> AArch64 machine instruction nodes
  -> optional .s printer output
  -> optional structured encoder/object input
```

Assembly text is a final output format for humans, debugging, snapshots, or an
external assembler. It is not an internal semantic handoff that codegen may ask
the assembler parser to recover. A future built-in encoder or object writer
must consume machine instruction nodes directly or consume a deliberately lower
structured encoding record derived from those nodes.

## Existing Surfaces This Contract Classifies

- `BACKEND_ENTRY_CONTRACT.md` owns the prepared-module entry requirement.
  Machine instruction nodes may consume only facts that flowed through BIR,
  `PreparedBirModule`, and AArch64 target records accepted by that contract.
- `module/module.hpp` owns target MIR snapshots of module, function, block,
  operand, frame, branch, call, move, spill/reload, ABI binding, data, string,
  symbol, and relocation-need facts. Those records preserve ids and prepared
  facts; display strings in those records are spelling or diagnostics only.
- `abi/abi.hpp` owns typed AArch64 register references such as
  `RegisterReference`, `RegisterBank`, and `RegisterView`, plus conversion from
  prepared physical-register spellings. `parse_aarch64_register_name(...)` is a
  conversion and diagnostic helper, not an assembler parser that supplies
  codegen semantics.
- `codegen/records.hpp` owns the current target-MIR record layer and the
  selected machine-node layer, including `InstructionFamily`, `OperandRecord`,
  `InstructionRecord`, branch, scalar, memory, spill/reload, call, return,
  assembler, and object record variants. The accepted selected subset covers
  structured branch, integer scalar ALU/cast, memory load/store, and prepared
  spill/reload pseudo nodes. The live `.s` printer consumes only the selected
  printable subset and fails closed for unsupported nodes; this document still
  does not require encoding, object writing, or linking behavior.
- `codegen/emit.hpp`, `assembler/mod.hpp`, `assembler/parser.hpp`,
  `assembler/types.hpp`, and `assembler/encoder/mod.hpp` remain compatibility,
  external-assembler, or legacy text-first surfaces until a later contract
  rebuilds them around structured machine nodes or encoding records.

## Machine Instruction Node Identity

A machine instruction node represents one target instruction-like operation or
one explicitly modeled pseudo operation after target MIR lowering has selected
an AArch64 instruction family. It must carry:

- an instruction family, such as branch, scalar, memory, call, return,
  assembler, object, or another family added by a later accepted contract
- either a concrete opcode or a family-specific opcode enum when the exact
  AArch64 instruction is selected
- a pseudo-instruction marker when the node is still a structured operation
  that a later lowering pass must expand before encoding
- ordered typed operands
- source metadata back to the BIR and prepared facts that authorized the node
- def, use, clobber, and side-effect metadata sufficient for validation,
  scheduling, late peepholes, register-sensitive checks, printers, and encoders

Mnemonic strings are not node identity. A printer may derive mnemonics from a
node, and diagnostics may include mnemonic text, but codegen and encoder logic
must branch on typed opcode or family identity.

## Naming Tiers And Alias Ownership

AArch64 naming is split into four tiers so codegen, printers, and future
encoding work do not accidentally treat one spelling as another semantic
surface:

- stream item kind names describe structured assembly-stream records such as
  sections, labels, operators/instructions, directives, data, alignment, and
  relocation needs.
- natural operator kind names are the MIR / structured-asm authoring surface.
  They should use readable AArch64 operator names when practical, including
  alias names such as `Mov`, `Cmp`, and `Tst` when the alias is the clearer
  semantic backend input.
- printer mnemonic names are terminal assembly spellings such as `mov`, `cmp`,
  or `tst` chosen by the `.s` printer for the supported printable subset.
  Printed `.s` remains final renderer output, not semantic backend input.
- encoder canonical form names are later concrete encoding-family choices used
  by assembler/encoder internals after alias canonicalization.

Aliases are valid at the machine-node or structured-asm layer when they make
the selected operation readable and preserve typed operands and provenance.
The MIR / structured-asm surface does not need to name every concrete encoding
form. The assembler/encoder canonicalization layer owns resolving aliases such
as `Mov`, `Cmp`, and `Tst` into concrete encodable forms, relocation behavior,
operand legality, and final encoding selection.

## Typed Operands

Machine instruction operands must be structured variants. A later C++ surface
may name these differently, but it must preserve these semantic categories:

- Register operand: an AArch64 `abi::RegisterReference` with bank, view, index,
  and role. When sourced from prepared allocation, it should retain the
  `PreparedValueId`, `ValueNameId`, prepared register class or bank, and
  expected view that justified the physical register.
- Immediate operand: a typed integer, boolean, null-pointer, or relocation
  immediate with width, signedness, source value identity when available, and
  range policy owned by the opcode or family.
- Memory operand: a structured address preserving base kind, optional base
  register, frame-slot id, symbol `LinkNameId`, pointer `PreparedValueId` or
  `ValueNameId`, string `TextId` or symbol id, byte offset, size, alignment,
  BIR address space, and volatility.
- Symbol operand: a `LinkNameId` or other accepted symbol id plus offset,
  visibility, relocation kind when known, and external/local classification.
- Branch-target operand: a `BlockLabelId` with function identity and optional
  condition/source metadata. Printed labels are display spellings only.
- Condition or predicate operand: structured branch, compare, condition-code,
  or predicate identity sourced from BIR opcodes or prepared branch facts.
- Prepared-value operand: `PreparedValueId`, `ValueNameId`, `TypeKind`,
  prepared value kind, home kind, storage encoding, register class or bank, and
  group width.
- Frame-slot operand: `PreparedFrameSlotId`, optional `PreparedObjectId`,
  `SlotNameId`, offset, size, alignment, and fixed-location policy.
- Data-reference operand: globals, string constants, data records, symbol
  visibility, TLS, initializer, and relocation-need references owned by the
  target MIR data/object side table.

Operand text, `AsmStatement::text`, comma-split operands, `raw_operands`, and
rendered register or value names cannot be the source of operand semantics for
nodes produced by codegen.

## Source Metadata

Every node should retain enough provenance to validate why it exists without
re-reading printed text. The required metadata is family-specific, but the
minimum accepted sources are:

- `FunctionNameId` and `BlockLabelId` for the owning function and block
- block index and instruction index when the node maps to a BIR instruction or
  terminator
- `PreparedValueId`, `ValueNameId`, and `TypeKind` for values used or defined
- source BIR opcode, cast opcode, terminator kind, call kind, or data/global
  identity where applicable
- prepared control-flow, branch-condition, parallel-copy, call-plan,
  value-location, regalloc, storage-plan, frame-plan, dynamic-stack,
  addressing, liveness, and data/object facts that authorize the node

Metadata may include display spellings for diagnostics and `.s` printing, but
the ids and prepared references are the lookup authority.

## Def, Use, Clobber, And Side-Effect Metadata

A machine instruction node must describe the semantic effects needed by later
target passes:

- defs: prepared values, physical registers, flags, memory destinations, frame
  slots, or object/data records written by the node
- uses: prepared values, physical registers, frame slots, symbols, memory
  addresses, branch conditions, predicates, call operands, and data references
  read by the node
- clobbers: caller-saved registers, call-clobbered resources, flags, scratch
  registers, memory barriers, and special architectural registers affected by
  the node
- side effects: volatile memory access, atomic or barrier behavior, calls,
  returns, traps, inline-assembly effects, externally visible data/object
  emission, and control-flow transfer

Calls must preserve the prepared call-plan authority for direct/indirect
callee identity, arguments, results, memory-return storage, preserved values,
variadic FP-register count, and clobbered registers. Memory nodes must preserve
prepared volatility and address-space facts. Branch nodes must preserve
prepared branch targets and compare/condition facts.

## Boundary Between Layers

Target MIR records are pre-node facts. They snapshot prepared module, operand,
frame, branch, call, move, spill/reload, data, and relocation needs. They may
contain display labels, but those labels are not node operands.

Machine instruction nodes are the post-selection or selected-pseudo instruction
representation. They are typed, ordered, provenance-bearing, and independent
of assembly text.

The `.s` printer is a consumer. The current public AArch64 route
`c4cll --codegen asm --target aarch64-linux-gnu input.c -o out.s` lowers
through prepared BIR, target MIR, selected machine nodes, and then prints a
GNU-style assembly translation unit for the selected printable subset. The
printer may choose final label spellings and print register names, but it must
not become the authority for later codegen or encoding facts.

The encoder/object path is also a consumer. It may consume nodes directly or a
lower structured encoding record derived from nodes. It must not consume
`parse_asm(...)`, `AsmStatement`, `Operand::text`, or `raw_operands` as the
codegen-owned semantic input.

The assembler parser is an external-input path. It may parse user-provided or
tool-provided assembly text when a later assembler contract accepts that work,
but it is not the bridge from AArch64 codegen to the built-in encoder/object
writer.

## Structured ASM/Encoding Record Surface

A future compile-through encoder/object route may introduce a deliberately
lower structured asm/encoding stream between machine instruction nodes and
object records. That stream is still structured data, not printed assembly
text. Its minimum accepted record families are:

- translation-unit record: owns the ordered stream, target triple or ABI
  identity, object-format policy, symbol table references, section table
  references, and provenance back to the prepared module.
- section record: owns section name or id, kind, flags, alignment, placement
  policy, contained labels/operators/directives/data, and section-local
  relocation needs.
- label record: owns structured function, block, data, local, or temporary
  label identity. Printed label spelling is display data only.
- operator/instruction record: owns opcode or pseudo identity, ordered typed
  operands, implicit defs/uses, clobbers, side effects, condition/predicate
  facts, and source machine-node provenance.
- directive record: owns structured assembler/object directives such as
  alignment, symbol visibility, symbol type/size, CFI-like metadata when
  accepted by a later contract, and object emission policy.
- data record: owns bytes, strings, zero-fill, constants, symbol associations,
  type/size/alignment facts, initializer provenance, and relocation needs.
- typed operand records: purpose-specific register, immediate, memory, symbol,
  label, condition, data-reference, and frame/value operands.
- relocation-need record: owns `RelocationNeed`-style target symbol/link-name
  identity, relocation kind, addend, offset owner, section owner, width, and
  provenance needed by object and linker records.

Typed operands may use a dispatch wrapper, but the semantic payload must remain
readable and purpose-oriented. A single broad catch-all operand payload is not
an accepted structured asm/encoding surface.

The accepted register operand decomposition preserves separate concerns. A
future implementation may choose different final C++ names, but it must retain
the split represented by this sketch:

```cpp
struct AsmRegisterRef {
  c4c::backend::aarch64::abi::RegisterReference reg;
  c4c::backend::aarch64::abi::RegisterView view;
};

enum class AsmRegisterUseKind {
  Input,
  Output,
  InOut,
  Scratch,
  Clobber,
  AddressBase,
  CallArgument,
  ReturnValue,
};

struct AsmRegisterUse {
  AsmRegisterUseKind kind;
  bool implicit = false;
};

struct AsmValueProvenance {
  std::optional<c4c::backend::prepare::PreparedValueId> value_id;
  c4c::ValueNameId value_name = c4c::kInvalidValueName;
  c4c::backend::bir::TypeKind type = c4c::backend::bir::TypeKind::Void;
};

struct AsmAllocationProvenance {
  c4c::backend::prepare::PreparedRegisterClass prepared_class =
      c4c::backend::prepare::PreparedRegisterClass::None;
  c4c::backend::prepare::PreparedRegisterBank prepared_bank =
      c4c::backend::prepare::PreparedRegisterBank::None;
  std::size_t contiguous_width = 1;
  std::vector<c4c::backend::aarch64::abi::RegisterReference> occupied_registers;
};

struct AsmRegisterOperand {
  AsmRegisterRef reg;
  AsmRegisterUse use;
  std::optional<AsmValueProvenance> value;
  std::optional<AsmAllocationProvenance> allocation;
};
```

Clobber and effect metadata must not be flattened into the register reference
or value provenance. It belongs on the operator/instruction record or on
purpose-specific effect records that describe whether a register is implicitly
read, written, clobbered, scratch-owned, call-preserved, call-clobbered, flag
affecting, memory affecting, volatile, trapping, returning, branching, or
externally visible.

The semantic payload that survives toward object/linker work includes typed
operands, register reference and use facts, immediate width/signedness/range
facts, symbol ids, `LinkNameId`, relocation kind and addend facts,
section/data ownership, branch and data references, volatility, BIR address
space, side effects, clobbers, source machine-node ids, prepared value/frame
ids, allocation provenance, and diagnostic spelling as non-authoritative
display metadata.

Prepared facts remain the upstream authority for long-lived semantic state.
Structured asm/encoding records may carry provenance back to
`PreparedLiveness`, `PreparedLiveInterval`, `PreparedRegalloc`,
`PreparedRegallocValue`, `PreparedInterferenceEdge`,
`PreparedMoveResolution`, `PreparedSpillReloadOp`,
`PreparedValueLocations`, `PreparedCallPreservedValue`, and
`PreparedClobberedRegister`, but they must not replace those facts with a
second allocation, liveness, call-preservation, or spill/reload policy. Those
prepared records decide value homes, interference, move obligations,
spill/reload authority, call-preserved values, and call-clobbered registers.

The structured asm/encoding layer adds only post-selection machine-level
effects that do not exist as prepared authority: implicit register uses and
defs introduced by the selected opcode, selected opcode clobber metadata,
condition flags, scratch lifetimes consumed by a selected lowering, final
operator side effects, and section/relocation ownership for the downstream
object stream. These additions attach to machine instruction nodes or lower
encoding records; they do not move semantic authority out of the machine
instruction node boundary and they do not authorize assembly-text recovery.

## Centralized Enum Spelling Contract

Future structured asm/encoding record surfaces must centralize every
enum-to-string or to_string mapping used for terminal printers and diagnostics.
Lowering, selection, encoder preparation, object preparation, and linker-facing
record construction must branch on typed enum values, not on scattered spelling
switches or string comparisons.

The centralized mappings must cover each enum kind introduced for section
records, label records, directive records, operator/opcode categories, operand
kind variants, register use kind variants such as `AsmRegisterUseKind`,
relocation kind variants, and record surface classifications. Terminal printers
and diagnostics call those mappings when they need display spelling; they do
not duplicate enum switch spelling through lowering code or treat printed text
as semantic input.

This docs-only plan has not introduced new live structured asm/encoding enum
types, so no live enum helper or implementation mapping is required in this
slice. The contract applies to the future C++ records when those enum surfaces
are added.

## Focused Guard Decision

The current live AArch64 `--codegen asm` route has no live route that feeds
printed `.s` from `machine_printer.cpp` into the in-tree
parser/encoder/object/linker path. The backend builds prepared machine nodes,
prints terminal assembly text with `print_machine_instruction_nodes(...)`, and
returns that text to the caller. The staged AArch64 `parse_asm`, `assemble`,
`encode_instruction`, object, and linker surfaces remain external-input or
future structured-record boundaries, not live internal consumers of that
printed output.

Because there is no accidental call chain from printed `.s` into AArch64
parser/encoder/object/linker code, this contract slice does not need a focused
guard test. A future implementation that creates such a live bridge must add
the narrow guard at that call chain or replace the bridge with structured
machine-node or asm/encoding records.

## Rejected Internal Routes

The following routes are rejected for AArch64 codegen:

- `codegen -> assembly text -> parse_asm -> encoder`
- deriving branch, symbol, value, frame-slot, memory, or register semantics
  from rendered names when ids or prepared facts exist
- using comma-split operands, raw operand tails, legacy markdown examples, or
  line-oriented peepholes as the semantic form for target instructions
- treating `.s` output as the contract that a built-in encoder or object
  writer must parse back into machine facts
- adding new instruction selection, printing, encoding, object writing, or
  linker behavior before the relevant structured node or encoding contract is
  accepted

## Current Alignment Boundary

The current implemented records are aligned around these boundaries:

- `InstructionFamily` remains family identity and can lead to concrete opcode
  identity without using mnemonic strings as authority.
- `OperandRecord` variants map to the typed operand categories in this
  contract.
- `InstructionRecord` and family records retain function, block, instruction,
  prepared-value, memory, branch, call, and data provenance.
- `RecordSurfaceKind::RecordOnly` remains only as a compatibility spelling for
  the target-MIR/pre-node surface. New structured selection output must use
  `MachineInstructionNode`, while future printer, encoder, object, and
  external assembler surfaces must use their explicit surface names.
- Selected machine nodes may carry `MachineOpcode`, `MachinePseudoKind`,
  `MachineNodeStatusRecord`, operands, def/use/clobber resources, and
  side-effect metadata. They are the source consumed by the `.s` printer for
  supported node families, but they still do not parse assembly text or imply
  encoder, object-writer, or linker support.
- assembler, object, and inline-assembly record variants do not imply that
  text-first assembler parsing is the internal semantic route.
