# Inline-Assembly Schema And Ownership Checkpoint

## Frozen separation of authorities

Three representations must remain distinct:

1. `opaque_template` is the decoded source byte payload. It is never escaped,
   aliased, substituted, or parsed before the late assembler.
2. `source_constraint` and structured source operand identity are the semantic
   input to the single LIR-to-BIR constraint normalizer.
3. `llvm_template`, `llvm_constraints`, and `llvm_args` are compatibility
   renderings used only by the LLVM printer/route. They cannot be read by the
   new backend, verifier, MIR lowering, or allocator.

The designated constraint-normalization owner is
`lower_lir_to_raw_bir`. It may inspect source constraint tokens and structured
clobber names under the closed table. It may not inspect `opaque_template`.
Mnemonic, directive, template-reference, and `.insn` interpretation belongs
only to `LateAssembler::assemble_inline_asm`.

## Required LIR carrier adjustment

Step 2 needs a carrier equivalent to:

```text
LirInlineAsmInput
  source_ordinal: uint32
  input_value: LirValueId
  type: LirTypeRef
  source_constraint: string
  source_name: optional<string>   # empty until parser support exists

LirInlineAsmOutput
  source_ordinal: uint32
  output_ordinal: uint32
  destination: LirValueId         # address/lvalue identity for writeback
  incoming_value: optional<LirValueId> # present only for UseDef
  result_type: LirTypeRef
  source_constraint: string
  source_name: optional<string>

LirInlineAsmResult
  output_ordinal: uint32
  result_index: uint32
  result_value: LirValueId        # produced SSA definition
  type: LirTypeRef

LirInlineAsmOp
  opaque_template: byte string
  outputs: vector<LirInlineAsmOutput>
  inputs: vector<LirInlineAsmInput>
  results: vector<LirInlineAsmResult>
  source_clobbers: vector<string>
  has_side_effects: bool
  llvm_compatibility: optional<LlvmInlineAsmRendering>
```

`source_ordinal` is the GCC template operand number: all outputs first, then
inputs. `output_ordinal` indexes `outputs`. `result_index` is the dense BIR/MIR
result position and is never inferred from either ordinal. For the admitted
register-only grammar every output has exactly one ordered result row;
`results[result_index].output_ordinal` is the authoritative mapping.

An output destination is neither an input value nor an asm result: it names
where the result is written after the asm. `Def` has no incoming value.
`UseDef` has an incoming SSA value and a distinct produced SSA result, plus the
same writeback destination. A numeric tied input retains its own input SSA
value and points to an `output_ordinal` for assignment equality; it never
aliases that output's produced result identity.

The compatibility record owns all `${...}`, escaping, rewritten constraint,
and preformatted argument text. The semantic verifier checks operand ordering,
value/type presence, unique output/result mappings, destination identity, and
clobber-vector shape, but does not parse constraint grammar or template bytes.
The active early `insn_r` metadata must be absent, not retained as optional
new-backend metadata.

## Target normalization provenance

`LirModule::target_profile` is the sole target authority at normalization. The
importer reads its `TargetArch`; it does not infer a target from spelling or
accept a caller override.

The BIR module stores only immutable provenance equivalent to:

```text
enum class InlineAsmConstraintContractId { Rv64RegisterGroupsV1 }

InlineAsmNormalizationProvenance
  target_arch: TargetArch
  constraint_contract: InlineAsmConstraintContractId
```

This is not an allocator profile: BIR stores no triple, ABI pools, reserved
register set, register names, or candidate order. Publishing inline asm
requires one module provenance record. `TargetArch::Unknown` fails import as
`UnknownInlineAsmTarget`; a known non-RV64 arch fails as
`UnsupportedInlineAsmTarget`. Both diagnostics retain source profile context
and publish no partial BIR.

BIR-to-MIR receives external `TargetRegisterInfo` bound to a concrete
`TargetProfile` and supported contract IDs. Before physical-clobber resolution
it requires BIR provenance, exact BIR/external arch agreement, and support for
the exact contract ID. Failures are `MissingInlineAsmProvenance`,
`BirMirTargetMismatch`, and `UnsupportedConstraintContract`, respectively. No
clobber is resolved and no MIR is published after failure. This prevents
cross-target reinterpretation without storing allocator policy in BIR.

## Target-independent BIR schema

Names below are frozen strongly enough to guide Step 2; normal repository
naming adjustments may not change their fields or ownership.

```text
enum class InlineAsmOperandRole { Use, Def, UseDef, TiedUse }
enum class InlineAsmRegisterClass { General, Vector }
enum class InlineAsmClobberKind { Memory, ConditionCodes, PhysicalRegister }

InlineAsmRegisterRequirement
  register_class: InlineAsmRegisterClass
  group_width: uint8             # 1, 2, 4, 8
  group_alignment: uint8         # equal to width in this checkpoint
  contiguous: bool               # always true in this checkpoint

InlineAsmPhysicalRegister
  register_class: InlineAsmRegisterClass
  physical_index: uint8          # generic unit index, target validated at import

InlineAsmInput
  source_ordinal: uint32
  input_value: ValueId
  requirement: InlineAsmRegisterRequirement
  tie_to_output_ordinal: optional<uint32>

InlineAsmOutput
  source_ordinal: uint32
  output_ordinal: uint32
  destination: ValueId           # writeback address/lvalue
  incoming_value: optional<ValueId>
  result_index: uint32
  role: Def | UseDef
  requirement: InlineAsmRegisterRequirement
  early_clobber: bool

InlineAsmResultSpec
  output_ordinal: uint32
  type: Type

InlineAsmClobber
  kind: InlineAsmClobberKind
  physical_register: optional<InlineAsmPhysicalRegister>

InlineAsmInst
  opaque_template: byte string
  inputs: vector<InlineAsmInput>
  outputs: vector<InlineAsmOutput>
  clobbers: vector<InlineAsmClobber>
  has_side_effects: bool

InlineAsmWritebackInst
  destination: ValueId
  produced_value: ValueId
  output_ordinal: uint32
```

These records contain semantic class, width, alignment, contiguity, ties, and
generic physical unit identity. They contain no mnemonic, opcode, `.insn`
fields, rendered constraint string, target register spelling, LLVM argument
text, or allocated home. `InlineAsmInst` becomes an ordinary BIR instruction
with normal stable `InstId` and result `ValueId` ownership.

The builder API is atomic and equivalent to
`append_inline_asm(block, InlineAsmInstSpec, vector<InlineAsmResultSpec>)`.
The spec never contains not-yet-created result `ValueId`s. The builder creates
one `InstId`, ordered result `ValueId`s as
`InstResultDef{instruction, result_index}`, attaches each result to its
declared `output_ordinal`, and creates ordered `InlineAsmWritebackInst` records
in the same transaction. It returns
`BuildInlineAsmResult { instruction, results, writebacks }`; any validation or
storage failure creates none of them.

Views expose immutable payload bytes, input/output mappings, ordered results,
and writebacks. Verification requires valid owner IDs, unique source/output
ordinals, a bijective output-to-result mapping, valid destinations, `Def`
without an incoming value, `UseDef` with distinct incoming/produced values,
ties to output assignments rather than result identities, legal
width/alignment, no duplicate physical clobbers, and no fallback.

## New MIR pseudo and constraint records

Step 3 needs a new active MIR independent of old prepared/MIR types:

```text
MirVRegId
MirInstId

MirRegisterRequirement
  register_class: General | Vector
  group_width: uint8
  group_alignment: uint8
  contiguous: bool

MirInlineAsmInput
  source_ordinal: uint32
  use_vreg: MirVRegId
  requirement: MirRegisterRequirement
  tie_to_output_ordinal: optional<uint32>

MirInlineAsmOutput
  source_ordinal: uint32
  output_ordinal: uint32
  incoming_use_vreg: optional<MirVRegId>
  produced_def_vreg: MirVRegId
  result_index: uint32
  requirement: MirRegisterRequirement
  early_clobber: bool

MirClobber
  kind: Memory | ConditionCodes | PhysicalUnits
  units: vector<PhysicalRegisterUnit>

MirInlineAsm
  opaque_template: byte string
  inputs: vector<MirInlineAsmInput>
  outputs: vector<MirInlineAsmOutput>
  clobbers: vector<MirClobber>
  has_side_effects: bool
```

BIR-to-MIR creates virtual registers and copies every normalized fact and byte.
It resolves generic physical clobbers through reviewed `TargetRegisterInfo`;
it does not parse source strings or template text.

Each output has a distinct def vreg. A `UseDef` also has a distinct incoming
use vreg; allocation equates their physical assignments, not their SSA/vreg
identities. A numeric tied input retains its use vreg and ties its assignment
to the selected output def. MIR writeback consumes the produced def vreg and
the separate destination identity.

## Allocator requirement and invariants

The allocator consumes, rather than merely prints, records equivalent to:

```text
AllocationRequirement
  vreg: MirVRegId
  register_class: General | Vector
  group_width: uint8
  group_alignment: uint8
  contiguous: bool
  tied_vreg: optional<MirVRegId>
  early_clobber_against: vector<MirVRegId>
  forbidden_units: set<PhysicalRegisterUnit>

PhysicalAssignment
  base: PhysicalRegisterUnit
  occupied_units: vector<PhysicalRegisterUnit>
```

`TargetRegisterInfo` owns the allocatable unit set, reserved units, physical
index/name conversion, and legal candidate enumeration. For every candidate
and final assignment:

- class must match;
- occupied count equals width;
- units are consecutive;
- base index is divisible by alignment;
- every unit is allocatable and absent from `forbidden_units`;
- tied operands have identical assignments;
- assignment equality never merges their SSA or MIR virtual identities;
- overlapping live operands interfere;
- `UseDef` covers both incoming and outgoing lifetime;
- an early-clobber def overlaps only its own tied input;
- a group is selected or rejected atomically.

Every normal, eviction, retry, and fallback allocation path must call the same
requirement-aware candidate filter. No spill or retry may weaken class, width,
alignment, tie, early-clobber, or clobber requirements. This explicitly avoids
the historical descriptive-only constraint failure documented in
`docs/pre_regalloc_value_constraints/`.

## Late assembler API

```text
LateAsmOperandAssignment
  source_ordinal: uint32
  register_class: General | Vector
  base_register: PhysicalRegisterUnit
  occupied_units: vector<PhysicalRegisterUnit>

LateAsmRequest
  target: TargetProfile
  opaque_template: byte string
  operands: vector<LateAsmOperandAssignment>

LateAsmError
  category: OperandReferenceOutOfRange |
            UnsupportedNamedOperand |
            UnsupportedTemplateModifier |
            TemplateModifierOperandMismatch |
            AsmParseError |
            AsmEncodingError
  byte_offset: size_t
  detail: string

LateAsmResult
  encoded_bytes: vector<uint8>
  relocations: vector<LateAsmRelocation>

LateAssembler::assemble_inline_asm(LateAsmRequest)
  -> Result<LateAsmResult, LateAsmError>
```

This function first substitutes the closed source placeholder grammar using
completed assignments, then invokes the target assembler parser/encoder. It
does not receive constraint strings, virtual registers, or allocation
requirements and cannot change assignments. This is the first call allowed to
recognize a mnemonic, directive, or `.insn`.

## Stage ownership matrix

| Stage | Owns | Must not own |
| --- | --- | --- |
| Parser/HIR | decoded payload bytes; source operand order/roles/values; original constraint token; clobber spelling | LLVM rendering as semantic truth; target class normalization; mnemonic/`.insn` parsing |
| HIR-to-LIR | structured source carrier; optional separate LLVM compatibility rendering | destructive replacement of source facts; semantic constraint authority |
| LIR verifier | carrier shape and value/type identity | class/group parsing; template parsing |
| LIR-to-BIR | sole normalization from `LirModule::target_profile`; atomic output/result/writeback construction; BIR target/contract provenance | compatibility strings; allocator profile; mnemonic/directive/`.insn` parsing |
| BIR verifier/views | input/output/result/writeback and role/class/group/tie/clobber invariants; immutable target/contract provenance | target spelling; pools/reserved registers; allocation; template parsing |
| BIR-to-MIR | target/contract match gate; distinct use/def vregs; target clobber-unit resolution; exact fact/byte copy | string parsing; cross-target reinterpretation |
| Regalloc | legal candidates, ties, liveness, interference, early-clobber and clobber exclusion | any string parsing; instruction encoding |
| Late assembler | source placeholder substitution followed by instruction parse/encode | constraint normalization; virtual allocation decisions |

## Direct proof matrix

| Boundary | Positive proof | Required negative proof |
| --- | --- | --- |
| Source/HIR to LIR | byte-equal invalid mnemonic payload; two ordered outputs retain destinations/results; original `+&r` differs from LLVM `=&r,N`; `UseDef` incoming/result IDs differ | reject missing value/type/destination/result mapping without inspecting payload; no `insn_r` metadata exists |
| LIR to BIR | admitted scalar/vector forms and clobbers; atomic multi-output results/writebacks; numeric tie plus `UseDef` proves source ordinal, output ordinal, and result index stay distinct; byte equality | every rejection includes category, operand/clobber index, spelling, target/profile, and contract context; unknown/non-RV64 target; `VRM1`; alternatives; type/width mismatch |
| BIR to MIR | ordered results/writebacks become distinct def vregs; `UseDef` incoming/produced vregs remain distinct; ties become assignment equality; provenance and bytes survive | missing provenance, unknown contract, mismatched/unknown external target, or bad result mapping fails before clobber resolution/MIR publication with structured context |
| Allocator | every width allocates legally; `UseDef` vregs share an assignment; numeric tied input shares output assignment without sharing result identity | pressure, overlap, misalignment, noncontiguity, early-clobber conflict, allocation-owned clobber conflict, and exhaustion carry instruction/operand context |
| Late assembler | completed assignments substitute and one minimal instruction encodes | invalid mnemonic and malformed `.insn` survive every early observation unchanged, with no `insn_r` metadata, then fail here; bad reference/modifier reports byte offset and operand context |

Each group-width proof must assert the complete occupied-unit vector, not only
the base spelling. Changing or removing a structured requirement must change
the allocator result or convert success to the matching failure; printer-only
observations are insufficient.

Structured-diagnostic proofs check payload, not category alone. Import errors
carry function/block/instruction context, optional operand/output or clobber
index, original spelling, `TargetArch`, source profile summary, and contract ID
when known. BIR-to-MIR errors carry BIR and external arch/contract values.
Allocation errors carry `MirInstId`, vregs, and source ordinals. Late errors
carry byte offset and referenced source ordinal.

## Known violations to remove before acceptance

- HIR currently rewrites `%N` placeholders before LIR.
- HIR currently parses `.insn r` and can reject it early.
- HIR-to-LIR currently rewrites an x86 mnemonic and LLVM-escapes the only
  payload field.
- HIR-to-LIR currently erases original `+` spelling into LLVM output-plus-tie.
- LIR currently stores operands as `args_str` rather than structured values.
- New BIR currently has no ordinary opcode and rejects inline asm wholesale.

These are implementation tasks, not ambiguities. No legacy source should be
re-enabled to remove them.

## Blockers and deliberately unresolved extensions

There is no blocker to the frozen RV64 `r`/vector group schema. The following
remain blocked extensions and must not be guessed into Step 2:

- named operands, because the parser discards their names;
- `VRM1`, because only quarantined code treats it as a `VR` alias;
- fixed-register operands, aliases, memory, immediate, alternative, and
  combined constraints, because they lack a reviewed admission table here;
- non-RV64 constraint and clobber rules;
- exact reserved GPR/vector policy, which must come from the new reviewed
  `TargetRegisterInfo` implementation rather than copied legacy pools.

Reviewer acceptance of this checkpoint is required before implementation.
