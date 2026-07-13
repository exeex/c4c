# Raw BIR Core Design Contract

Status: **closed architecture contract; implementation remains incomplete**.
This file is deliberately more complete than the bootstrap
implementation. It is not a claim that the headers in this directory already
implement the APIs below.

This contract defines the storage and public API of `RawBir`, the first owned,
verified IR produced after LIR import. It is intended to be reviewed together
with the ordered pass contracts. A decision marked **Target** is part of the
accepted replacement architecture; a statement marked **Confirmed current
fact** describes code that exists today. Missing source carriers or missing
implementation are recorded as such and are not unresolved architecture.

## 1. Scope and authority

### 1.1 Core owns

Core is the single authority for:

- a module's resolved semantic types, constants, link-visible symbols, globals,
  string/data objects, aliases, declarations, and definitions;
- the immutable semantic data-layout facts needed to interpret those types and
  objects: byte order, pointer width/alignment per address space, and resolved
  object/record size and alignment. These are not ABI value classifications;
- each function's signature, parameters, semantic automatic-storage objects,
  blocks, ordered instructions, terminators, values, and source attachments;
- stable IDs, allocation/tombstone state, explicit semantic iteration order,
  exact def-use, module/function revisions, and mutation summaries;
- operation facts that later passes must transform or preserve: scalar and
  aggregate operations, memory and atomic semantics, direct/indirect calls,
  `va_start`/`va_arg`/`va_copy`/`va_end`, target-independent intrinsics,
  inline-assembly intent, fixed/dynamic allocation, stack save/restore and
  lifetime operations, and control-flow successors;
- enough semantic layout information to interpret object byte ranges and
  aggregate fields. This is source/LIR-resolved object layout, not an ABI
  register/stack classification.

`RawBir` may contain non-canonical forms accepted only at the import boundary.
The canonical pass pipeline consumes it in a fixed order and eventually
produces `CanonicalBir` over the same core storage.

### 1.2 Stage ownership inside core

The following are explicitly outside **Raw/Canonical core storage**:

- abstract register homes, register classes chosen for allocation,
  spill/reload decisions, spill-slot identities, live intervals, coalescing,
  or parallel-copy schedules;
- frame indices, stack offsets, saved-register slots, prologue/epilogue policy,
  red zones, call-frame sizes, or concrete stack-pointer adjustments;
- ABI argument/result classification, assigned argument registers, incoming or
  outgoing stack offsets, `sret` placement, by-value copy placement, HFA/eightbyte
  placement, or call-preservation moves;
- selected target opcodes, addressing modes, GOT/TLS materialization policy,
  relocation encoding, instruction encodings, target register spellings, or
  emission order;
- authoritative predecessor, dominator, loop, liveness, value-home,
  provenance-proof, publication-route, comparison-route, or return-chain tables;
- LIR parsing strings, compatibility name maps, prepared records, target emitter
  objects, and testcase-shaped route facts.

The semantic data layout is the minimum target-derived input required for
byte-correct IR. It may say that a pointer in address space N is 32 or 64 bits;
it may not say which register class carries that pointer, how an aggregate is
classified at a call boundary, or which relocation/opcode realizes its address.

Analyses may derive CFG predecessors, dominance, liveness, memory effects, or
provenance from core and bind those results to a revision. Target preparation
may read canonical core and produce revision-bound capacity, ABI-eligibility,
class/group, tie, early-clobber, and clobber-exclusion facts. Original asm
constraint/clobber tokens remain semantic Raw/Canonical payload; normalized
target meaning does not. Allocation liveness and interference are exact-revision
`E1` BIR analysis products; chosen abstract homes are `E2` BIR allocation
products; and abstract spill-object identities plus explicit `Spill`/`Reload`
nodes are owned by the private `E3` Pseudo BIR candidate. None may be written
back into Raw/Canonical core storage. Concrete registers, frame locations,
target operations, and encodings remain MIR/backend facts after verified `E4`
publication.

### 1.3 Dependency boundary

Core headers may depend only on generic support libraries and other core
headers. They must not include LIR, legacy BIR, prealloc, preparation, MIR, or
target-emitter headers. Importers translate into builder specifications. Passes
and analyses consume views/editors; they never gain access to storage internals.

## 2. Confirmed bootstrap facts versus target

The partial implementation is useful scaffolding, but it is not the finished
schema:

| Area | Confirmed current fact | Target consequence |
|---|---|---|
| IDs | `ids.hpp` defines epoch/owner/slot/generation IDs for functions, blocks, instructions, and parameter/instruction-result values. | Preserve the identity model and extend it to types, constants, symbols, globals, locals, and attachments. |
| Storage | `storage.hpp` has generation-checked `SlotMap`, tombstones, free slots, and independent `IdOrder`. | Preserve observable semantics; hide all mutable storage behind builders/editors. |
| Types | `type.hpp` has only `Void`, small integers, `F32/F64`, and opaque `Pointer`. | Replace with an interned, layout-capable type graph covering `I128/F128`, arrays, records, vectors, and function types. |
| Instructions | `ir.hpp` currently defines only `Opcode::InlineAsm`. Each instruction has generic ordered `ValueId` operands/results plus an `InlineAsmNode`; no general opcode family exists yet. | Extend the closed semantic opcode/payload and descriptor registry without weakening the generic value-edge contract. |
| Control flow | Four terminators exist and successors are derived from the terminator. | Retain this authority rule and add switch, indirect branch, and asm-goto successor forms. |
| Views | Current views expose functions, parameters, blocks, values, instruction IDs, terminators, and successors. | Extend to every entity and descriptor while keeping references mutation-scoped. |
| Construction | `ModuleBuilder::with_function` gives a scoped `FunctionBuilder`; bootstrap `publish()` runs only the foundation verifier. | Retain capability scoping, but replace direct publication with `ModuleDraft -> full Raw verification -> RawBir`; add globals/types/constants and complete errors. |
| Mutation | No general editor, def-use store, local/global schema, or revision exists in current core. | The APIs below are target APIs, not existing behavior. |

### 2.1 Confirmed current inline-assembly slice

The implemented bootstrap slice deliberately separates value transport from
opaque source payload:

- `InlineAsmSpec::inputs` is an ordered vector of ordinary function-local
  `ValueId`s. `result_types` creates an ordered vector of instruction-result
  `ValueId`s whose `InstResultDef` points back to the instruction and exact
  result index. `InstView::operands()` and `InstView::results()` expose those
  same generic edges; there is no parallel inline-asm value-ID table.
- `InlineAsmNode` stores only the original asm text, original constraint text,
  clobber spellings, and `side_effects`. The two strings are opaque semantic
  payload at this layer: core does not parse them into register classes, ties,
  target opcodes, or allocation facts.
- `FunctionBuilder::append` rejects foreign or unresolved inputs and `void`
  results. If result allocation or block-order insertion fails, it erases the
  instruction and every result created by that append before returning an
  error.

Asm-goto topology and the later revision-bound constraint interpretation,
register allocation, spill/reload, and MIR forms described later in this
document remain unimplemented, not architecturally unresolved. Parsed
constraint objects are deliberately not future Raw/Canonical core state.

## 3. Ownership graph

```text
RawBir (move-only stage proof)
└── ModuleData [ModuleEpoch, ModuleRevision, RegistryVersion]
    ├── SemanticDataLayout: byte order + per-address-space pointer layout
    ├── TypeStore:        SlotMap<TypeData, TypeId> + structural interning
    ├── TypeNameStore:    SlotMap<TypeNameData, TypeNameId>
    ├── ConstantStore:    SlotMap<ConstantData, ConstantId> + interning
    ├── InitializerStore: SlotMap<GlobalInitializer, InitializerId>
    ├── SymbolTable:      SlotMap<SymbolData, SymbolId> + link-name index
    ├── DirectiveStore:   SlotMap<ModuleDirective, DirectiveId>
    ├── GlobalStore:      SlotMap<GlobalData, GlobalId> + global order
    ├── FunctionStore:    SlotMap<FunctionData, FunctionId> + function order
    ├── TopLevelOrder: Symbol/Global/Function/Directive entries
    ├── DebugStore:       files, scopes, locations, names
    └── OriginStore:      non-authoritative source/import provenance

FunctionData [FunctionRevision]
├── signature and semantic attributes
├── parameters: ordered ValueId list
├── locals:      SlotMap<LocalData, LocalId> + declaration order
├── blocks:      SlotMap<BlockData, BlockId> + layout order
├── insts:       SlotMap<InstData, InstId>
├── values:      SlotMap<ValueData, ValueId>
└── uses:        DefUseStore keyed by ValueId

BlockData
├── ordered InstId list
└── exactly one TerminatorData after publication

InstData
├── Opcode + typed semantic payload
├── ordered result ValueIds
├── source/debug/origin attachments
└── no owning block pointer (membership is the block order authority)
```

An entity has exactly one owner. A function-local ID can never be resolved in
another function. Module-owned `SymbolId`, `GlobalId`, `TypeId`, and
`ConstantId` are distinct namespaces; a `ValueId` never aliases any of them.
`RawBir` is an unforgeable stage proof, not a typedef or a public aggregate:
only the successful Raw verification gate can construct it from a consumed
`ModuleDraft`. Storage that is still being built or edited is never a `RawBir`.

## 4. Stable IDs, storage, and order

### 4.1 Proposed ID family

```cpp
using ModuleEpoch = std::uint64_t;
using ModuleRevision = std::uint64_t;
using FunctionRevision = std::uint64_t;
using RegistryVersion = std::uint64_t;
using SlotIndex = std::uint32_t;
using Generation = std::uint32_t;

struct TypeId       { ModuleEpoch epoch; SlotIndex slot; Generation generation; };
struct TypeNameId   { ModuleEpoch epoch; SlotIndex slot; Generation generation; };
struct ConstantId   { ModuleEpoch epoch; SlotIndex slot; Generation generation; };
struct InitializerId{ ModuleEpoch epoch; SlotIndex slot; Generation generation; };
struct SymbolId     { ModuleEpoch epoch; SlotIndex slot; Generation generation; };
struct GlobalId     { ModuleEpoch epoch; SlotIndex slot; Generation generation; };
struct DirectiveId  { ModuleEpoch epoch; SlotIndex slot; Generation generation; };
struct DebugFileId  { ModuleEpoch epoch; SlotIndex slot; Generation generation; };
struct DebugScopeId { ModuleEpoch epoch; SlotIndex slot; Generation generation; };
struct DebugLocId   { ModuleEpoch epoch; SlotIndex slot; Generation generation; };
struct OriginId     { ModuleEpoch epoch; SlotIndex slot; Generation generation; };
struct FunctionId   { ModuleEpoch epoch; SlotIndex slot; Generation generation; };
struct BlockId      { FunctionId owner; SlotIndex slot; Generation generation; };
struct InstId       { FunctionId owner; SlotIndex slot; Generation generation; };
struct LocalId      { FunctionId owner; SlotIndex slot; Generation generation; };

enum class ValueKind : std::uint8_t {
  Parameter,
  InstResult,
};
struct ValueId {
  FunctionId owner;
  ValueKind kind;
  SlotIndex slot;
  Generation generation;
};
using ModuleEntityId = std::variant<TypeId, TypeNameId, ConstantId, InitializerId,
    SymbolId, GlobalId, DirectiveId,
    FunctionId, DebugFileId, DebugScopeId, DebugLocId, OriginId>;
using FunctionEntityId = std::variant<LocalId, BlockId, InstId, ValueId>;
using EntityId = std::variant<ModuleEntityId, FunctionEntityId>;
```

IDs are opaque semantic identities, not vector positions, names, instruction
numbers, labels, or prepared value IDs. Zero epoch/generation is invalid. Slot
reuse increments generation before reuse. A generation at its maximum is
retired permanently rather than wrapping. A new module instance receives a new
epoch; an ID from a destroyed or different module fails resolution.

### 4.2 Lifetime guarantees

- Reordering functions, blocks, locals, or instructions does not change IDs.
- Growing/compacting storage may move objects in memory without changing IDs.
- Moving an instruction between blocks preserves its `InstId` and result
  `ValueId`s when the edit remains within the same function.
- Erasure tombstones an ID. The same slot at a newer generation is a different
  entity and stale IDs return `ResolveError::StaleGeneration`.
- `Handle<T>` may retain `{shared module lifetime, ID}` across commits and
  resolves every access. A `Ref<T>`, range iterator, `string_view`, or payload
  reference carries a borrow token and expires on the next structural mutation
  of its owner.
- Persistent caches store IDs plus the exact module/function revision, never
  object addresses, iterators, dense analysis indexes, or instruction offsets.

### 4.3 Deterministic iteration order

Allocation order is not observable. Explicit ID-order lists define:

1. one top-level order of symbol/global/function/alias/directive/data-object
   entries, plus the filtered symbol/global/function orders derived from it;
2. function parameter and local declaration order;
3. function block layout order;
4. block instruction order;
5. instruction result and operand order;
6. terminator successor-slot order (`true,false`; switch cases in their stored
   deterministic semantic order followed by default; indirect/asm targets in
   declared order). The slot role and ordinal form semantic edge identity.

An editor method always names an insertion position. Printing, deterministic
diagnostics, serialized snapshots, and preparation use these orders. Analysis
may build private dense indexes, but they cannot escape the analysis result.

## 5. Type, symbol, global, and constant schema

### 5.1 Resolved types

**Target:** scalar, pointer, array, vector, function, and anonymous aggregate
types are immutable and structurally interned. Named record types are nominal:
the builder reserves one `TypeId`/`TypeNameId`, defines its body exactly once
after recursive references have been reserved, and it becomes immutable at
draft finish. Equality is `TypeId` equality within one module epoch. Signedness
is an operation property for most integer arithmetic, but source-required
signedness may be retained as a debug qualification; it must not create
inconsistent bit-width semantics.

```cpp
enum class TypeKind : std::uint8_t {
  Void, Integer, Float, Pointer, Array, Vector, Record, Function
};
enum class FloatFormat : std::uint8_t {
  IeeeBinary16,
  IeeeBinary32,
  IeeeBinary64,
  X87Extended80,
  IeeeBinary128,
};
enum class RecordKind : std::uint8_t { Struct, Union };
enum class AddressSpaceKind : std::uint8_t { Default, Fs, Gs, Tls, TargetNamed };
struct AddressSpace {
  AddressSpaceKind kind;
  std::uint32_t target_number; // zero unless TargetNamed
};

enum class ByteOrder : std::uint8_t { Little, Big };
struct PointerLayout {
  AddressSpace space;
  std::uint16_t bit_width;
  std::uint16_t abi_align_bytes;
};
struct FloatLayout {
  FloatFormat format;
  std::uint16_t semantic_bit_width;
  std::uint16_t storage_size_bytes;
  std::uint16_t abi_align_bytes;
};
struct SemanticDataLayout {
  ByteOrder byte_order;
  std::vector<PointerLayout> pointers;
  std::vector<FloatLayout> floats;
  std::string target_triple; // semantic environment key, not opcode/ABI policy
};

struct IntegerType { std::uint16_t bit_width; };
struct FloatType { FloatFormat format; };
struct PointerType { AddressSpace space; }; // opaque; GEP carries source element type
struct ArrayType { TypeId element; std::uint64_t count; };
struct VectorType { TypeId element; std::uint32_t lane_count; bool scalable; };
struct RecordField { TypeId type; std::uint64_t byte_offset; std::optional<std::string> debug_name; };
struct RecordType {
  RecordKind kind;
  std::optional<TypeNameId> tag;
  std::vector<RecordField> fields;
  std::uint64_t size_bytes;
  std::uint32_t align_bytes;
  bool packed;
  bool opaque;
};
enum class ParameterListKind : std::uint8_t {
  Prototype, Variadic, Unspecified
};
struct FunctionType {
  TypeId result;
  std::vector<TypeId> parameters;
  ParameterListKind parameter_list_kind;
};
using TypePayload = std::variant<IntegerType, FloatType, PointerType, ArrayType,
                                 VectorType, RecordType, FunctionType>;
struct TypeData { TypeKind kind; TypePayload payload; };
```

`IntegerType{1}` is the boolean type; signedness remains on operations.
Resolved aggregate offsets/size/alignment are semantic importer facts because
memory operations and global initializers need byte-accurate object meaning.
They do not say how an aggregate is split into ABI registers. Pointer bit width
comes from `SemanticDataLayout`; relocation encoding and target instruction
realization remain later. Core code must not derive target register classes from
`TypeData` or from the semantic layout.

### 5.2 Symbols and functions

```cpp
struct ObjectLayout { std::uint64_t size_bytes; std::uint32_t align_bytes; };
enum class SymbolKind : std::uint8_t { Function, Global, Alias };
enum class Linkage : std::uint8_t { External, Internal, Private, Common, Weak, LinkOnce };
enum class Visibility : std::uint8_t { Default, Hidden, Protected };
enum class CallingConvention : std::uint8_t { C, SysV, Win64, Fast, Cold, TargetNamed };
enum class TlsModelRequest : std::uint8_t {
  Default, GeneralDynamic, LocalDynamic, InitialExec, LocalExec
};

struct SymbolAttributes {
  Linkage linkage;
  Visibility visibility;
  std::optional<std::string> section;
  std::optional<std::string> version;
  bool thread_local;
  TlsModelRequest tls_model_request;
  bool used;
};
struct SymbolData {
  SymbolKind kind;
  std::string link_name;       // unique spelling, not entity identity
  SymbolAttributes attributes;
  std::optional<SymbolId> alias_target;
};
struct FunctionAttributes {
  CallingConvention source_calling_convention;
  bool no_return;
  bool returns_twice;
  bool no_unwind;
  bool naked;
  bool inline_hint;
  bool always_inline;
  bool no_inline;
  std::optional<std::uint16_t> constructor_priority;
  std::optional<std::uint16_t> destructor_priority;
};
enum class ExtensionKind : std::uint8_t { None, SignExtend, ZeroExtend };
struct ParameterAttributes {
  ExtensionKind extension;
  bool by_value;
  bool structure_return_pointer;
  std::optional<ObjectLayout> by_value_layout;
};
struct ReturnAttributes { ExtensionKind extension; };
struct FunctionData {
  SymbolId symbol;
  TypeId function_type;
  std::vector<ParameterAttributes> parameter_attributes;
  ReturnAttributes return_attributes;
  FunctionAttributes attributes;
  bool is_declaration;
  // owned stores and orders described above
};
```

The calling-convention and parameter/result attributes record explicit
source/LIR constraints (`byval`, semantic hidden-result pointer, sign/zero
extension). They do
not contain classified locations. Function declarations have no blocks or
locals; definitions have one entry block after publication. Redeclarations
merge only when symbol, type, linkage, and semantic attributes agree.

### 5.3 Globals, locals, and data objects

```cpp
struct GlobalData {
  SymbolId symbol;
  TypeId type;
  ObjectLayout layout;
  enum class DefinitionKind : std::uint8_t {
    Declaration, Tentative, Common, Definition
  } definition_kind;
  bool is_constant;
  bool has_explicit_alignment;
  std::optional<InitializerId> initializer;
  std::optional<DebugLocId> debug;
  std::optional<OriginId> origin;
};
struct LocalData {
  TypeId type;
  ObjectLayout layout;
  bool address_taken;
  bool is_volatile;
  std::optional<std::string> debug_name;
  std::optional<DebugLocId> debug;
  std::optional<OriginId> origin;
};
```

A `LocalId` denotes a semantic automatic-storage object, not a frame slot.
`local_addr(LocalId)` creates a pointer value. Lifetime markers and dynamic
allocation operations may refer to the object, but no frame offset exists in
core. Static locals are module globals with internal symbols.
String-pool entries and other emitted constant data are ordinary private or
internal `GlobalData` definitions with byte-fragment initializers; core does not
create a second data-object authority beside globals.

Top-level assembly and linker-visible directives are ordered module records,
not printer side channels:

```cpp
struct TopLevelAsmSpec {
  std::string source_text;
  std::vector<SymbolId> symbol_dependencies;
  OriginId origin;
};
struct TopLevelAsmRecord {
  std::string source_text;
  std::vector<SymbolId> symbol_dependencies;
  OriginId origin;
};
struct SymbolVersionDirective { SymbolId symbol; std::string version_expression; };
using ModuleDirective = std::variant<TopLevelAsmRecord,
    SymbolVersionDirective>;
using TopLevelItem = std::variant<GlobalId, FunctionId, SymbolId, DirectiveId>;
```

Aliases are `SymbolData{Alias, alias_target}` and appear in top-level order as
their alias `SymbolId`; no directive duplicates their target authority. The
directive records preserve other source requests and stable symbol references. They do
not contain assembler output order chosen by a target emitter, parsed target
opcodes, relocation encodings, or allocation facts. Constructor/destructor
membership and priority are function attributes and are realized later.

`TopLevelAsmSpec::symbol_dependencies` is the ordered, producer-supplied set of
semantic symbol references used by the assembly payload. The stored
`TopLevelAsmRecord` copies that order exactly. Reachability, validation, and
symbol retention use only these `SymbolId`s; core never scans or parses
`source_text` to discover dependencies.

### 5.4 Constants and initializers

```cpp
struct IntegerBits { std::vector<std::byte> little_endian_bits; };
struct FloatBits { FloatFormat format; std::vector<std::byte> exact_bits; };
struct SymbolAddressConstant { SymbolId symbol; std::int64_t byte_addend; };
struct BlockAddressConstant { FunctionId function; BlockId block; };
struct AggregateConstant { std::vector<ConstantId> elements; };
enum class SpecialConstant : std::uint8_t { Zero, Null, Undef, Poison };

using ConstantPayload = std::variant<IntegerBits, FloatBits,
    SymbolAddressConstant, BlockAddressConstant, AggregateConstant,
    SpecialConstant>;
struct ConstantData { TypeId type; ConstantPayload payload; };

enum class RelocationSemantic : std::uint8_t {
  ObjectAddress, FunctionAddress, ThreadLocalAddress, BlockAddress
};
using RelocationTarget = std::variant<SymbolId, BlockAddressConstant>;
struct InitBytes { std::vector<std::byte> bytes; };
struct InitZero { std::uint64_t byte_count; };
struct InitConstant { ConstantId value; };
struct InitRelocation {
  RelocationTarget target;
  std::int64_t addend;
  std::uint8_t width_bytes;
  RelocationSemantic semantic;
};
struct InitLabelDifference {
  BlockAddressConstant lhs;
  BlockAddressConstant rhs;
  std::uint8_t width_bytes;
};
using InitFragmentPayload = std::variant<InitBytes, InitZero, InitConstant,
    InitRelocation, InitLabelDifference>;
struct InitFragment { std::uint64_t byte_offset; InitFragmentPayload payload; };
struct GlobalInitializer { std::vector<InitFragment> fragments; };
```

Integer and floating constants preserve exact bits, including `I128` and
binary128. Global initialization is a sparse, offset-explicit object image; it
supports zero-fill, scalar/aggregate constant fragments, byte strings (narrow,
wide, and UTF-16 are bytes with resolved element layout), padding,
symbol-plus-addend relocations, block addresses, and computed-goto label
differences. Fragment ranges may not overlap unless a future union-specific
rule explicitly permits it. Relocation target, semantic, width, and addend are
core facts; ELF/COFF relocation kind and object-file encoding are later
target/emission decisions.

`Undef` and `Poison` are distinct: `Undef` permits an arbitrary value selected
per semantic use under the canonical rules, while `Poison` propagates invalid
optimization state until a defined consuming boundary. Neither may be silently
materialized as zero. `Null` is legal only for a pointer type; exact bit-vector
length is dictated by the integer width or `FloatFormat`.

### 5.5 Debug and source provenance hooks

Debug and import provenance are attachments, never identity or optimization
authority:

```cpp
struct DebugFile { std::string path; std::optional<std::string> directory; };
struct DebugScope {
  std::optional<DebugScopeId> parent;
  std::optional<SymbolId> function_or_object;
  std::optional<std::string> display_name;
};
struct DebugLocation {
  DebugFileId file;
  DebugScopeId scope;
  std::uint32_t line;
  std::uint32_t column;
  std::optional<DebugLocId> inlined_at;
};
enum class OriginKind : std::uint8_t {
  LirModule, LirFunction, LirBlock, LirInst,
  SynthesizedByImporter, SynthesizedByPass
};
struct OriginData {
  OriginKind kind;
  std::optional<std::uint64_t> producer_stable_id;
  std::optional<std::string> diagnostic_spelling;
  std::optional<OriginId> parent;
};
```

`DebugFileId` and `DebugScopeId` follow the same module-owned
epoch/slot/generation model as `DebugLocId`. Passes may drop, merge, or compose
locations under an explicit debug-preservation policy. `OriginData` exists for
diagnostics and migration traceability; a verifier, pass, or preparation stage
must not use producer spelling/position as semantic identity. Memory provenance
is not stored here: it is derived from object IDs and pointer-producing
instructions by a revision-keyed analysis.

This is the complete Raw/Canonical v1 debug/origin schema. Object debug types
are reconstructed from the entity's semantic `TypeId`, resolved layout, symbol
or local attachment, and the scope/location graph; core does not maintain a
second debug-type identity graph. Richer lexical, macro, discriminator, or
source-language type detail may be dropped at import and cannot affect
verification. `OriginId` is the sole import/pass provenance link; parallel
origin arrays and producer positions are not alternate authority. This bounded
retained subset is sufficient for v1 object debug output, while richer output
is an implementation/source-carrier extension rather than an architecture
prerequisite.

## 6. Function IR schema

### 6.1 Values, operands, and definitions

```cpp
struct ParameterDef { std::uint32_t ordinal; };
struct InstResultDef { InstId instruction; std::uint16_t result_index; };
using ValueDefinition = std::variant<ParameterDef, InstResultDef>;
struct ValueData {
  TypeId type;
  ValueDefinition definition;
  std::optional<std::string> debug_name;
  std::optional<OriginId> origin;
};

enum class OperandKind : std::uint8_t { Value, Constant, Symbol, Local, BlockAddress };
using Operand = std::variant<ValueId, ConstantId, SymbolId, LocalId, BlockAddressConstant>;
```

Ordinary computed values are always `ValueId`s. Constants and symbol references
are explicit operands rather than fake named values. This removes the legacy
ambiguity where a string beginning with `@` sometimes names a global, a
function, or a string constant. An opcode descriptor specifies which operand
kinds and types are legal for each role.

### 6.2 Semantic opcode families

The enum is closed for a build: adding an opcode requires payload, descriptor,
printer, importer, traversal, verifier, and pass-policy coverage. Proposed
families are:

```cpp
enum class Opcode : std::uint16_t {
  // scalar and aggregate
  Copy, Unary, Binary, Compare, Cast, Select, Phi,
  ExtractValue, InsertValue, BuildAggregate,
  ExtractElement, InsertElement, ShuffleVector,

  // addresses and object lifetime
  LocalAddr, GlobalAddr, FunctionAddr, BlockAddr,
  PtrOffset, GetElementPtr, LifetimeStart, LifetimeEnd,
  DynamicAlloc, StackSave, StackRestore,

  // memory
  Load, Store, MemCopy, MemMove, MemSet,
  AtomicLoad, AtomicStore, AtomicRmw, AtomicCompareExchange, Fence,

  // calls and language/runtime semantics
  Call, VarArgStart, VarArg, VarArgCopy, VarArgEnd,
  Intrinsic, InlineAsm,

  // raw-only semantic operations that canonical passes must legalize
  WideIntegerOp, ExtendedFloatOp, ComplexOp, CheckedOverflowOp,
};
```

This table is the Raw/Canonical semantic set; it does not admit `Spill`,
`Reload`, target opcodes, or physical carriers. Runtime-helper-
capable behavior remains a precise ordinary operation (`Binary`, `Cast`,
`Compare`, `WideIntegerOp`, `ExtendedFloatOp`, or `ComplexOp`). A legality
descriptor may require later expansion, but Raw BIR never replaces semantics
with a vague helper candidate, helper spelling, or calling sequence.

Representative payloads:

```cpp
enum class MemoryEffect : std::uint8_t {
  None, Read, Write, ReadWrite, Allocate, Fence, Unknown
};
struct IntrinsicId { std::uint32_t value; }; // interpreted only with RegistryVersion
enum class UnaryOp : std::uint8_t { Neg, BitNot, LogicalNot, FNeg };
enum class BinaryOp : std::uint8_t {
  Add, Sub, Mul, UDiv, SDiv, URem, SRem, And, Or, Xor, Shl, LShr, AShr,
  FAdd, FSub, FMul, FDiv, FRem
};
enum class CompareOp : std::uint8_t {
  Eq, Ne, Ult, Ule, Ugt, Uge, Slt, Sle, Sgt, Sge,
  FOeq, FOne, FOlt, FOle, FOgt, FOge, FOrd, FUno
};
enum class CastOp : std::uint8_t {
  Trunc, ZExt, SExt, FPTrunc, FPExt, FPToUI, FPToSI, UIToFP, SIToFP,
  PtrToInt, IntToPtr, Bitcast
};
struct MemoryAccess {
  TypeId access_type;
  std::uint32_t align_bytes;
  AddressSpace address_space;
  bool is_volatile;
};
enum class AtomicOrdering : std::uint8_t { Relaxed, Acquire, Release, AcqRel, SeqCst };
enum class AtomicRmwOp : std::uint8_t {
  Exchange, Add, Sub, And, Nand, Or, Xor, Min, Max, UMin, UMax
};

enum class SuccessorRole : std::uint8_t {
  JumpTarget, TrueTarget, FalseTarget, SwitchCase, SwitchDefault,
  IndirectPossibleTarget, AsmFallthrough, AsmGotoTarget
};
struct EdgeKey {
  BlockId source;
  SuccessorRole role;
  std::uint32_t successor_index;
};
struct PhiPayload { std::vector<EdgeKey> incoming_edges; };
struct GepPayload {
  TypeId source_element_type;
  std::vector<TypeId> index_types;
  bool inbounds;
};
struct DynamicAllocPayload { TypeId element_type; std::uint32_t align_bytes; };
struct MemoryTransferPayload {
  std::uint32_t destination_align_bytes;
  std::uint32_t source_align_bytes;
  bool is_volatile;
};

enum class TailRequest : std::uint8_t { None, Tail, MustTail };
enum class ReturnBehavior : std::uint8_t {
  ReturnsOnce, ReturnsTwice, NeverReturns
};
enum class UnwindBehavior : std::uint8_t { CannotUnwind, MayUnwind };
enum class CallMemoryEffect : std::uint8_t {
  None, ReadOnly, WriteOnly, ReadWrite, Unknown
};
struct CallEffects {
  ReturnBehavior return_behavior;
  UnwindBehavior unwind_behavior;
  CallMemoryEffect memory_effect;
  bool convergent;
  bool cannot_duplicate;
};
enum class OperandBundleKind : std::uint8_t {
  Deopt, Funclet, GcTransition, Assume
};
struct CallOperandBundle {
  OperandBundleKind kind;
  std::uint32_t first_bundle_operand;
  std::vector<TypeId> operand_types;
};
struct CallSiteAttributes {
  TailRequest tail_request;
  CallEffects effects;
  std::vector<ParameterAttributes> argument_attributes;
  ReturnAttributes return_attributes;
};
struct CallPayload {
  TypeId callee_type;
  CallingConvention source_calling_convention;
  std::uint32_t fixed_argument_count;
  ParameterListKind parameter_list_kind;
  CallSiteAttributes attributes;
  std::vector<CallOperandBundle> operand_bundles;
};
struct VarArgPayload { TypeId requested_type; std::optional<ObjectLayout> aggregate_layout; };
struct IntrinsicPayload {
  IntrinsicId semantic_intrinsic;
  std::vector<TypeId> type_arguments;
  std::vector<std::uint64_t> immediate_arguments;
  MemoryEffect declared_effect;
};
struct InlineAsmPayload {
  std::string original_asm_text;
  std::string original_constraint_text;
  std::vector<std::string> original_clobber_spellings;
  bool side_effects;
};
```

The remaining supporting closed descriptor types are:

```cpp
struct NoPayload {};
struct UnaryPayload { UnaryOp operation; };
struct BinaryPayload { BinaryOp operation; };
struct ComparePayload { CompareOp operation; };
struct CastPayload { CastOp operation; TypeId source_type; TypeId target_type; };
struct AggregatePayload {
  TypeId aggregate_type;
  std::vector<std::uint32_t> indices;
};
struct MemoryPayload { MemoryAccess access; };
enum class AtomicResultForm : std::uint8_t {
  None, LoadedValue, OldValue, NewValue, BooleanSuccess, OldAndSuccess
};
struct AtomicPayload {
  std::optional<MemoryAccess> access; // absent only for Fence
  std::optional<AtomicRmwOp> operation;
  AtomicOrdering success_ordering;
  std::optional<AtomicOrdering> failure_ordering;
  bool weak_compare_exchange;
  AtomicResultForm result_form;
};
enum class RuntimeSemanticOp : std::uint16_t {
  WideDivide, WideRemainder, ExtendedFloatArithmetic,
  ExtendedFloatConversion, ComplexArithmetic
};
struct RuntimeSemanticPayload {
  RuntimeSemanticOp operation;
  TypeId semantic_type;
};
using InstPayload = std::variant<NoPayload, UnaryPayload, BinaryPayload,
    ComparePayload, CastPayload, AggregatePayload, PhiPayload, GepPayload,
    DynamicAllocPayload, MemoryPayload, MemoryTransferPayload, AtomicPayload,
    CallPayload, VarArgPayload, IntrinsicPayload, InlineAsmPayload,
    RuntimeSemanticPayload>;
```

`RuntimeSemanticOp` is semantic, not a helper-name registry. The intrinsic
registry entry supplies complete type/immediate arity, effects, purity, and
required semantic feature. Target feature availability is checked by
legalization/preparation rather than hidden inside the numeric ID.

Core owns one closed, build-versioned semantic intrinsic registry. A module
records its `RegistryVersion`, and `IntrinsicId` denotes only the entry at that
version; registry entries own the stable semantic name, operand/result and
immediate schema, conservative effects, portable feature class, and required
legalization disposition. P07 owns alias normalization into those canonical
IDs. Unknown IDs, mismatched versions, and ISA-opcode identities fail closed.
An operation with target-independent semantics may carry a required-feature
tag, but selected instructions, helpers, and target support decisions remain
later-stage products.

Fixed automatic objects use `LocalId` plus `LocalAddr`; they never imply a frame
slot. `DynamicAlloc` has one descriptor-visible count/byte-size operand and a
pointer result, while its payload records element type and requested alignment.
`StackSave` has no operands and one pointer result; `StackRestore` consumes
exactly that pointer family and has no result. Their ordering and lifetime
effects are semantic, but concrete stack-pointer arithmetic, frame offsets, and
unwind realization are deferred. `LifetimeStart`/`LifetimeEnd` describe object
liveness and cannot substitute for stack save/restore.

`VarArgStart`, `VarArg`, `VarArgCopy`, and `VarArgEnd` are explicit operations,
not magic calls or generic intrinsics. They expose every va-list address,
requested result type, and aggregate size/alignment through descriptors and
payload; GP/FP save areas, overflow offsets, and ABI classes remain external
preparation results.

`CallEffects` and `CallSiteAttributes` are closed semantic call-site facts.
`ReturnsTwice` covers constructs such as `setjmp`; unwind and memory effects are
explicit and cannot be inferred from a callee spelling. Bundle values are
ordinary descriptor-visible operands and participate in exact def-use. Bundle
kind/type/range cardinality is verified against the closed schema. None of
these fields assigns an ABI class, argument register, stack offset, call move,
or physical return location.

Inline assembly uses the ordinary instruction value graph. Its ordered generic
operands are the inputs/uses and its ordered generic results are the
definitions; a read/write position therefore has an incoming value and a
distinct produced value rather than a collapsed identity. The payload retains
only the original opaque asm text, original opaque constraint text, ordered
clobber spellings, and side-effect flag. Core does not parse those strings,
derive ties/classes, assign registers, or translate a physical spelling to a
target register number. The later constraint-binding stage interprets the
original constraint text against these exact operand/result orders and
revision-bound target tables. Renderer text and parsed constraint objects are
not Raw/Canonical core authority.

### 6.3 Instruction and descriptor contract

```cpp
struct InstData {
  Opcode opcode;
  InstPayload payload;
  std::vector<Operand> operands;
  std::vector<ValueId> results;
  std::optional<DebugLocId> debug;
  std::optional<OriginId> origin;
};

enum class OperandRole : std::uint16_t {
  Lhs, Rhs, Source, Condition, TrueValue, FalseValue,
  Address, DestinationAddress, SourceAddress, StoredValue, Size, Count, FillByte,
  Callee, CallArgument, CallBundleOperand, PhiIncoming, AtomicExpected, AtomicDesired,
  AsmInput, AsmOutputAddress, IntrinsicArgument, DynamicSize,
  ReturnValue, BranchCondition, SwitchSelector, IndirectTarget
};
struct OperandSlot {
  InstId user;
  OperandRole role;
  std::uint16_t index;
  Operand value;
};
struct ResultSlot { InstId definition; std::uint16_t index; ValueId value; TypeId type; };
enum class SlotCardinality : std::uint8_t { ExactlyOne, Optional, Variadic };
enum class TypeRule : std::uint8_t {
  AnyNonVoid, ExactPayloadType, SameAsOperand, Integer, Float, Pointer,
  Boolean, Aggregate, DescriptorCallback
};
struct OperandDescriptor {
  OperandRole role;
  SlotCardinality cardinality;
  std::uint32_t allowed_operand_kinds_mask;
  TypeRule type_rule;
  std::uint16_t related_slot;
};
struct ResultDescriptor {
  SlotCardinality cardinality;
  TypeRule type_rule;
  std::uint16_t related_slot;
};
struct OpcodeDescriptor {
  Opcode opcode;
  std::span<const OperandDescriptor> operands;
  std::span<const ResultDescriptor> results;
  MemoryEffect memory_effect;
  bool has_side_effects;
  bool raw_only;
  bool may_trap;
};
```

The registry also supplies a bounded payload-kind check and an enumerator for
typed non-operand references such as `PhiPayload::incoming_edges`. The shared
semantic visitor therefore sees every `TypeId`, `SymbolId`, `LocalId`,
`BlockId`/`EdgeKey`, operand, and result even when it does not participate in
value def-use. `DescriptorCallback` is a typed registry function selected by
the closed opcode, not an arbitrary plugin or string hook.

Every variable-arity family still exposes all slots through the descriptor
protocol. Call arguments and operand-bundle values, phi incoming values, intrinsic arguments, inline-asm
inputs/output addresses, memory addresses, and atomic values cannot hide in an
unvisited payload. Results are similarly complete, including explicitly
multi-result non-call operations such as compare-exchange or checked overflow.
Calls and returns remain zero-or-one semantic value as specified below.

Payload vectors carry only non-operand metadata. For example, `PhiPayload` edge
key `i` describes operand slot `PhiIncoming[i]`; every call has exactly one
`Callee` operand (`SymbolId` for a direct call, pointer-valued operand for an
indirect call), followed by typed `CallArgument` slots and the flattened
`CallBundleOperand` ranges named by `CallPayload::operand_bundles`. GEP indices,
stack-restore pointers, memory-transfer addresses/sizes,
and every variadic operand are ordinary descriptor-visible slots. No operand
may be hidden in an opcode-specific payload.

### 6.4 Terminators and successors

```cpp
struct JumpTerm { BlockId target; };
struct CondJumpTerm { Operand condition; BlockId true_target; BlockId false_target; };
struct SwitchCase { ConstantId value; BlockId target; };
struct SwitchTerm { Operand value; std::vector<SwitchCase> cases; BlockId default_target; };
struct IndirectJumpTerm { Operand target; std::vector<BlockId> possible_targets; };
struct ReturnTerm { std::optional<Operand> value; };
struct AsmGotoTerm { InstId asm_instruction; BlockId fallthrough; std::vector<BlockId> targets; };
struct UnreachableTerm {};
using TerminatorData = std::variant<JumpTerm, CondJumpTerm, SwitchTerm,
    IndirectJumpTerm, ReturnTerm, AsmGotoTerm, UnreachableTerm>;

struct SuccessorSlot { BlockId source; SuccessorRole role; std::uint32_t index; BlockId target; };
```

The terminator is the only CFG successor authority. A `SuccessorSlot` also has
the derived `EdgeKey{source,role,index}` used by phi incoming entries, so two
switch/asm slots from one block to the same target remain distinct without an
independently mutable edge table. `AsmGotoTerm` makes
inline-asm control edges visible without letting an ordinary instruction own a
hidden CFG. Its `asm_instruction` must be the final ordinary instruction in the
same block, must be `Opcode::InlineAsm`, and its label constraints must match
the terminator targets one-for-one when the later constraint-binding product is
formed. The stored target vector itself is the ordered typed topology; Raw does
not recover or authorize targets by parsing constraint text. `ReturnTerm` has no value for a void
`FunctionType` and exactly one type-equal semantic value otherwise. Aggregate
and complex returns are one typed aggregate value; physical return lanes are
not representable in core. A `Call` likewise produces zero results for void and
exactly one result of its semantic function result type otherwise.

Asm-goto results use the paired ordinary-instruction model, not terminator-
defined edge values. Every result is defined by the final `InlineAsm`
instruction and is therefore available on each successor edge under the same
dominance rule as any other instruction result. The `AsmGotoTerm` owns only
ordered fallthrough/goto topology. A producer form whose outputs have
edge-specific definitions or availability cannot be represented by v1 and
must fail import; it must not be approximated with duplicated values, hidden
edge payload, or parsed constraint state.

Raw/Canonical v1 has no in-function exception, invoke, cleanup-pad, or landing-
pad edge. `CallEffects::MayUnwind` means an exception may escape the current
function; it does not create a hidden successor. A producer call requiring a
local unwind destination is an explicit source/schema gap and fails import.
If a later architecture admits handled exceptions, core terminators and their
ordered successor slots must own those edges and P03/P04 must process their
`EdgeKey`s; no call payload or exception side table may become CFG authority.

### 6.5 SSA choice

**Target (closed):** use explicit `Phi` instructions with incoming
`(EdgeKey incoming_edge, Operand value)` pairs because legacy BIR, LIR import, and
the reference backend already expose this form. Core must not also implement
block arguments as a simultaneous authority. Phi operands participate in
def-use; their values dominate the
corresponding predecessor edge, not the phi's block body.

For a phi block, authority is the exact incoming `SuccessorSlot`/`EdgeKey`
**multiset**, not a set of predecessor blocks or destination blocks. Each
derived incoming successor slot occurs exactly once in every phi and each phi
entry names one such slot; parallel switch or asm-goto edges from the same
source therefore remain separate even when their destination is identical.
Reordering or redirecting successor slots must update phi edge keys in the same
transaction.

`EdgeKey` has exactly the stored schema `{source BlockId, SuccessorRole,
successor_index}`. The role and index together are the typed successor-slot
ordinal used by P03/P04; destination block, predecessor number, rendered label,
and vector position elsewhere are never edge identity. Raw permits valid
single-definition SSA values and explicit phis; P04 is the sole owner of the
canonical phi placement/order and promotion promises. Block arguments are not
an alternate v1 representation.

## 7. Def-use contract

```cpp
struct InstOperandUse { InstId user; OperandRole role; std::uint16_t index; };
struct TerminatorUse { BlockId user_block; OperandRole role; std::uint16_t index; };
using UserRef = std::variant<InstOperandUse, TerminatorUse>;
struct UseRecord { ValueId used_value; UserRef user; };
```

- Every operand slot containing a `ValueId` has exactly one reciprocal
  `UseRecord`; constants, symbols, locals, and block addresses are not entered
  into the function value-use store.
- `users(ValueId)` is canonical, eager state maintained by builders/editors.
  A verifier can independently traverse every operand and compare exact sets.
- Result ownership is immutable: result `i` of an instruction has definition
  `{InstId,i}`. Moving the instruction preserves the definition; replacing it
  requires an explicit result map.
- A value cannot be erased while live uses remain. RAUW is typed, same-function,
  atomic, includes phi and terminator users, and leaves the old use-list empty.
- Module symbol-reference indexes, if cached, are revision-bound analyses rather
  than a second semantic use authority.
- New operation kinds cannot land until descriptor traversal and def-use audit
  tests cover every value-bearing slot.

## 8. Raw construction state and builder APIs

### 8.1 State machine

```text
empty ModuleBuilder
  -> module declarations/types/constants/symbols reserved
  -> function construction transactions (temporarily incomplete)
  -> all forward references resolved
  -> consumed ModuleDraft + read-only CandidateModuleView
  -> full Raw verification
  -> move-only RawBir
```

Builders may contain incomplete functions, unterminated blocks, and reserved
entities. These states are not observable through `RawBir`. Forward references
use reserved stable IDs, not string lookups or unresolved pointer tokens.
Finishing a builder is not publication. It yields an unpublished `ModuleDraft`;
the verifier reads only its `CandidateModuleView`. The verifier gate consumes
the draft and constructs `RawBir` only on success. On failure it destroys or
returns no usable candidate. No public constructor, cast, deserializer, or
builder method can manufacture `RawBir` from unverified storage.

`ModuleBuilder::draft_view()` is a diagnostic borrow of mutable, potentially
incomplete construction state. It expires on the next builder mutation and may
not be passed to analyses or treated as publication-ready. After `finish()`
consumes the builder, `ModuleDraft::view()` is a frozen read-only candidate view:
its IDs/order no longer change, but it remains unverified and cannot escape as
`RawBir`. The verifier consumes that exact frozen draft atomically.

### 8.2 Proposed concrete API

```cpp
template<class T> using BirResult = Result<T, BirError>;

class ModuleDraft;          // move-only, unpublished storage
class CandidateModuleView;  // read-only borrow of ModuleDraft
class ModuleView;           // read-only view of a verified stage object
class VerifiedRawToken;     // constructible only by verifier implementation
class RawPublicationGate;
class PublicationFailure;   // concrete definition is owned by verify contract
class ModuleDraft {
public:
  ModuleDraft(ModuleDraft&&) noexcept;
  CandidateModuleView view() const;
private:
  friend class RawPublicationGate;
};
class RawBir {
public:
  RawBir(RawBir&&) noexcept;
  RawBir& operator=(RawBir&&) noexcept;
  ModuleView view() const;
private:
  friend class RawPublicationGate;
  RawBir(VerifiedRawToken, ModuleDraft&&); // verifier gate only
};

class ModuleBuilder {
public:
  static BirResult<ModuleBuilder> create(ModuleBuildOptions);

  BirResult<TypeId> intern_type(TypeSpec);
  BirResult<std::pair<TypeNameId, TypeId>> reserve_named_record(TypeNameSpec);
  BirResult<void> define_named_record(TypeId, RecordDefinitionSpec);
  BirResult<ConstantId> intern_constant(ConstantSpec);
  BirResult<InitializerId> add_initializer(GlobalInitializer);
  BirResult<SymbolId> declare_symbol(SymbolSpec);
  BirResult<GlobalId> declare_global(GlobalSpec);
  BirResult<void> define_global(GlobalId, GlobalDefinitionSpec);
  BirResult<FunctionId> declare_function(FunctionDeclSpec);
  BirResult<void> add_alias(SymbolId alias, SymbolId target);
  BirResult<void> add_top_level_asm(TopLevelAsmSpec);
  BirResult<void> add_symbol_directive(SymbolDirectiveSpec);

  template<class Fn>
  BirResult<void> with_function(FunctionId, Fn&&); // Fn(FunctionBuilder&) -> BirResult<void>

  CandidateModuleView draft_view() const;
  BirResult<ModuleDraft> finish() &&;
};

struct BuildResult { InstId instruction; std::vector<ValueId> results; };
struct InstSpec {
  Opcode opcode;
  InstPayload payload;
  std::vector<Operand> operands;
  std::vector<TypeId> result_types;
  std::optional<DebugLocId> debug;
  std::optional<OriginId> origin;
};

class ReservedInst {
public:
  ReservedInst(ReservedInst&&) noexcept;
  ReservedInst& operator=(ReservedInst&&) noexcept;
  ReservedInst(const ReservedInst&) = delete;
  ReservedInst& operator=(const ReservedInst&) = delete;
  InstId instruction() const;
  std::span<const ValueId> results() const;
private:
  friend class FunctionBuilder;
  FunctionId owner_;
  InstId instruction_;
  std::vector<ValueId> results_;
  std::vector<TypeId> result_types_;
  std::uint64_t reservation_nonce_;
};

class FunctionBuilder {
public:
  FunctionId id() const;
  BirResult<ValueId> parameter(std::uint32_t ordinal) const;
  BirResult<LocalId> create_local(LocalSpec);
  BirResult<BlockId> create_block(BlockPosition,
                                  std::optional<std::string> debug_name = {},
                                  OriginId = {});
  BirResult<ReservedInst> reserve_instruction(
      BlockId, InstPosition, std::span<const TypeId> result_types);
  BirResult<BuildResult> define_reserved_instruction(
      ReservedInst&&, InstSpec);
  BirResult<BuildResult> append(BlockId, InstSpec);
  BirResult<BuildResult> insert(BlockId, InstPosition, InstSpec);
  BirResult<void> set_terminator(BlockId, TerminatorSpec);
  BirResult<void> replace_terminator(BlockId, TerminatorSpec);
  BirResult<void> attach_debug(EntityId, DebugLocId);
  BirResult<void> attach_origin(EntityId, OriginId);
};

```

`reserve_instruction` immediately allocates the final `InstId`, allocates each
result `ValueId` with its final `{instruction,result_index}` definition and
type, and inserts the instruction ID at the named block-order position. It
stores an internal undefined reservation and returns the only move-only
`ReservedInst` capability for that reservation. This permits phi shells
and other forward references to use final IDs without names or temporary IDs.

`define_reserved_instruction` consumes the token. Its `InstSpec::result_types`
must match the reserved result count and every reserved `TypeId` exactly; it may
not change the owner, `InstId`, result IDs, block membership, or order. Operand,
payload, and descriptor validation then defines the reserved instruction
atomically. A wrong/expired token or mismatch leaves the reservation undefined
and returns an error. Dropping a token does not erase or auto-define its
reservation: `ModuleBuilder::finish`, candidate verification, and publication
all fail if any reserved instruction is undefined. Ordinary `append`/`insert`
are convenience transactions that reserve and define in one operation.

The neighboring verifier owns the only publication entry point:

```cpp
Result<RawBir, PublicationFailure> verify_and_publish_raw(ModuleDraft&&);
```

For an importer, the intended pattern is: declare all link symbols; intern
types; reserve/decorate every function; create all blocks; create locals and
instructions; set terminators; finish; verify; publish. Names are accepted only when declaring
symbols or debug attachments. All relationships after reservation use IDs.

## 9. Views, traversal, and mutation APIs

### 9.1 Read-only views

```cpp
class ModuleView {
public:
  ModuleEpoch epoch() const;
  ModuleRevision revision() const;
  IdRange<TypeId> types() const;
  IdRange<TypeNameId> type_names() const;
  IdRange<SymbolId> symbols() const;
  IdRange<GlobalId> globals() const;
  IdRange<FunctionId> functions() const;
  BirResult<TypeRef> type(TypeId) const;
  BirResult<TypeNameRef> type_name(TypeNameId) const;
  BirResult<ConstantRef> constant(ConstantId) const;
  BirResult<InitializerRef> initializer(InitializerId) const;
  BirResult<SymbolRef> symbol(SymbolId) const;
  BirResult<GlobalRef> global(GlobalId) const;
  BirResult<FunctionView> function(FunctionId) const;
  BirResult<SymbolId> find_link_name(std::string_view) const;
};

class FunctionView {
public:
  FunctionId id() const;
  FunctionRevision revision() const;
  SymbolId symbol() const;
  TypeId function_type() const;
  bool is_declaration() const;
  IdRange<ValueId> parameters() const;
  IdRange<LocalId> locals() const;
  IdRange<BlockId> blocks() const;
  BirResult<LocalRef> local(LocalId) const;
  BirResult<BlockRef> block(BlockId) const;
  BirResult<InstRef> instruction(InstId) const;
  BirResult<ValueRef> value(ValueId) const;
  BirResult<TerminatorRef> terminator(BlockId) const;
};

IdRange<InstId> instructions(FunctionView, BlockId);
OperandRange operands(FunctionView, InstId);
ResultRange results(FunctionView, InstId);
UseRange users(FunctionView, ValueId);
SuccessorRange successors(FunctionView, BlockId); // derived from terminator
```

Ranges yield descriptors containing stable IDs and roles. They do not expose
mutable vectors. `predecessors` is intentionally absent from core traversal; it
comes from revision-keyed CFG analysis.

### 9.2 Function editor transaction

```cpp
class FunctionEditor {
public:
  FunctionView view() const;
  BirResult<BuildResult> insert_before(InstId, InstSpec);
  BirResult<BuildResult> insert_after(InstId, InstSpec);
  BirResult<BuildResult> append(BlockId, InstSpec);
  BirResult<BuildResult> replace_inst(InstId, InstSpec, ResultMapping);
  BirResult<void> erase_inst(InstId);
  BirResult<void> move_before(InstId moved, InstId anchor);
  BirResult<void> move_to_end(InstId moved, BlockId destination);
  BirResult<void> set_operand(OperandSlot, Operand replacement);
  BirResult<void> replace_all_uses_with(ValueId from, Operand to);
  BirResult<void> set_terminator(BlockId, TerminatorSpec);
  BirResult<BlockId> split_block(BlockId, InstId before);
  BirResult<void> redirect_edge(EdgeKey edge, BlockId new_to,
                                PhiTransferPlan);
  BirResult<void> erase_block(BlockId, BlockErasePlan);
  BirResult<LocalId> create_local(LocalSpec);
  BirResult<void> erase_local(LocalId);
  BirResult<MutationSummary> commit();
  void rollback() noexcept;
};
```

Only one editor may exist for a function. A failed operation leaves the
transaction usable; a failed commit rolls back. While the transaction is open,
its candidate storage is visible only through a `CandidateModuleView`; the
verified stage view continues to observe the old snapshot. Commit invokes the
stage-appropriate `verify_after_edit` over the mutation closure and swaps the
candidate into the stage object only on success. It increments
`FunctionRevision` exactly once and returns changed entities/effect categories.
`split_block` preserves moved instruction/value IDs,
transfers the old terminator, installs a jump, and updates successor phi keys.
`redirect_edge` requires an explicit phi transfer plan; it cannot guess values.

### 9.3 Module editor barrier

```cpp
class ModuleEditor {
public:
  BirResult<TypeId> intern_type(TypeSpec);
  BirResult<ConstantId> intern_constant(ConstantSpec);
  BirResult<GlobalId> add_global(GlobalSpec);
  BirResult<FunctionId> add_function(FunctionDeclSpec);
  BirResult<void> replace_global_initializer(GlobalId, GlobalInitializer);
  BirResult<void> change_symbol(SymbolId, SymbolEditSpec);
  BirResult<void> erase_global(GlobalId);
  BirResult<void> erase_function(FunctionId);
  BirResult<ModuleMutationSummary> commit();
  void rollback() noexcept;
};
```

Module edits run at a pipeline barrier with no live function editors. They bump
`ModuleRevision`; affected function revisions are bumped only when their
semantic body/signature changes. Their candidate is also stage-verified before
atomic swap. Reordering does not invalidate IDs.

## 10. Revision and concurrency contract

- Distinct functions may be transformed concurrently after the module's type,
  symbol, global, and function tables are frozen for the phase.
- Each worker receives an immutable `ModuleSymbolView`, one exclusive
  `FunctionEditor`, and its function analysis manager. It cannot add or mutate
  module entities.
- A module pass stops all workers, destroys borrowed views/ranges, obtains one
  `ModuleEditor`, commits, then starts a new parallel phase.
- A committed function mutation increments `FunctionRevision` once regardless
  of the number of internal edits. A debug-name-only mutation is classified
  separately and may preserve semantic analyses.
- `MutationSummary` distinguishes operand/result changes, instruction
  insert/erase/reorder, memory-effect changes, terminator/edge changes,
  block/local/signature changes, and attachment-only changes.
- Analysis results carry `{FunctionId, FunctionRevision}` or
  `{ModuleEpoch, ModuleRevision}`. A module-wide result that reads function
  bodies (for example a call graph) additionally carries the exact ordered
  `{FunctionId, FunctionRevision}` snapshot or an equivalent checked digest;
  `ModuleRevision` alone covers only module-owned tables. Any mismatch is a hard
  stale-result error, not a hint to trust cached data.
- Diagnostics are collected per function and merged in module function order so
  parallel execution remains deterministic.

## 11. Error model

Core never reports a semantic construction failure only as `false`, null, or a
free-form string.

```cpp
enum class BirErrorCode : std::uint16_t {
  WrongEpoch, WrongOwner, OutOfRange, Tombstone, StaleGeneration, WrongKind,
  StorageExhausted, RevisionExhausted, CapabilityExpired, ActiveEdit,
  DuplicateSymbol, ConflictingDeclaration, DuplicateDefinition,
  InvalidType, TypeMismatch, InvalidConstant, InvalidInitializer,
  InvalidOperandKind, InvalidOperandArity, InvalidResultArity,
  InvalidBlock, InvalidInstruction, InvalidValue, InvalidLocal,
  MissingTerminator, TerminatorAlreadySet, InvalidSuccessor,
  InvalidPhiIncoming, LiveUsesRemain, DominanceViolation,
  UnsupportedSemanticForm, UnresolvedForwardReference,
  UndefinedReservedInstruction, VerificationFailed
};
struct BirError {
  BirErrorCode code;
  std::optional<EntityId> entity;
  std::optional<OperandRole> role;
  std::string message;
  std::optional<OriginId> origin;
};
```

`BirResult<T>` is the narrow core layer for storage, resolution, and builder or
editor operations. `PublicationFailure` is one concrete verifier-owned boundary
type defined by the neighboring verify contract; core only forward-declares it
for `verify_and_publish_raw` and does not define or mirror a second shape. The
publication layer returns `Result<RawBir, PublicationFailure>`. The LIR
importer then wraps either failure in its own
`Result<ImportedRawBir, ImportFailure>` so it can add `ImportReport` and source
coordinates; it does not replace or duplicate core/verifier error taxonomies.

Internal impossible states may assert in debug builds but must also be caught by
verification. Batch import may accumulate diagnostics, but no invalid `RawBir`
is published. Unsupported source features fail closed with the originating
entity and semantic form; they must not be rewritten into a weaker operation
merely to pass one testcase.

## 12. Publication invariants

`verify_and_publish_raw(std::move(draft))` with the Raw profile checks at least:

1. every live ID has the correct epoch/owner/generation/kind; every order entry
   resolves and every owned live entity appears exactly once where required;
2. symbol names are unique under linkage rules; aliases resolve without illegal
   cycles; top-level asm dependency IDs resolve in their stored order;
   declaration/definition merges agree in type and semantic attributes;
3. types are well formed, records have valid field ranges, object layouts cover
   initializer bytes/relocations, and constants match their type exactly;
4. declarations have no body; definitions have a valid entry block; every block
   has exactly one terminator and every successor belongs to the same function;
   every return has zero operands for void or one value exactly matching the
   `FunctionType` result, never physical lanes;
5. no undefined instruction reservation remains; every instruction has
   descriptor-valid payload, operand/result arity, types, effects, and result
   definitions; every instruction appears in one block;
6. every reciprocal def-use record is exact; erased IDs have no uses; ordinary
   definitions dominate uses and phi inputs dominate predecessor edges;
7. each phi incoming `EdgeKey` multiset equals the exact derived incoming
   `SuccessorSlot` multiset, including parallel edges; switch cases and
   indirect/asm target lists are valid;
8. direct callees are `SymbolId` operands and indirect callees are typed pointer
   operands; calls match structural function types, fixed/variadic argument
   counts, zero-or-one semantic result, closed effects/call-site attributes,
   bundle ranges/types, and source calling-convention constraints without
   computing ABI homes;
9. loads/stores/atomics have valid pointer/address spaces, widths, alignment,
   volatile and ordering rules; aggregate indices/ranges are in bounds;
10. inline asm exposes every ordinary input and output through the generic
    operand/result graph and preserves its original opaque asm text, original
    opaque constraint text, ordered clobber spellings, and side-effect flag;
    Raw verification does not parse target constraint meaning; intrinsics
    expose typed operands, immediates, results, and declared effects;
11. debug/origin IDs resolve, but missing debug data never changes semantics;
12. no LIR spelling map, legacy route/prealloc record, frame/register/ABI plan,
    target opcode, emitted relocation kind, or raw persistent pointer is stored.

Raw verification permits only enumerated raw-only opcodes. Canonical verification
later requires those forms to have been normalized or explicitly retained by
the canonical profile.

## 13. Legacy-to-new coverage map

This table maps semantic responsibility, not field-for-field preservation.
Legacy route and prepared artifacts are often observations or target policy;
their *inputs* must remain expressible while the artifacts themselves do not
belong in core.

| Legacy source anchor | Existing responsibility | New core coverage | Deferred/rejected legacy state |
|---|---|---|---|
| `src/backend/legacy/bir.hpp: TypeKind`, `Value` | scalar types; immediate/named values; exact F128 payload | interned integer/float/pointer/vector types; typed `ConstantId`; `ValueId` definitions | VRM register grouping and spelling-based identity are rejected from core |
| `bir.hpp: Module`, `Function`, `Block`, `Terminator` | direct ownership and CFG body | stable module/function/block/inst/value stores; explicit order; terminator-only successors | raw vectors, label/name identity, instruction-index authority |
| `bir.hpp: Global`, `StringConstant`, `GlobalInitializerRelocationSlot` | globals, bytes, pointer initializers, TLS/const/layout facts | `GlobalData`, byte/zero/relocation/label-difference initializers, symbol IDs, exact object layout | `GlobalAddressMaterializationPolicy` is preparation/MIR policy |
| `bir.hpp: StructuredTypeSpellingContext`; `src/backend/bir/lir_to_bir/lowering.hpp: AggregateTypeLayout` | record spelling and importer-resolved layout | interned array/record/vector types with field offsets/size/alignment | LIR type strings and parity/fallback maps stay importer-private |
| `bir.hpp: Param`, `LocalSlot`, `Function` | parameters, semantic locals, signature, declaration flag | parameter `ValueId`s, `LocalId`, `FunctionType`, semantic attributes | ABI classes, incoming stack offsets, sret/byval placement, frame slots |
| `bir.hpp: BinaryInst`, `SelectInst`, `CastInst`, `PhiInst` | scalar ops, comparisons, select, casts, joins | typed semantic opcode/payload, explicit operands/results, phi `(EdgeKey,Operand)` | same-block producer/route snapshots become analyses or disappear |
| `bir.hpp: LoadLocalInst`, `LoadGlobalInst`, `StoreLocalInst`, `StoreGlobalInst`, `MemoryAddress` | object access, offsets, alignment, address space, volatility, provenance observations | `LocalAddr`/`GlobalAddr` + pointer ops + generic typed load/store and semantic access attributes | base-name fallback, target addressing form, prepared stack address |
| `src/backend/legacy/bir_memory_provenance.hpp: MemoryAccessProvenance`; `bir_local_array_semantic_gep.hpp` | object/range/path proof records | core preserves explicit objects, GEP/ptr-offset derivations, effects, source origins | proof/status/index records are revision-keyed provenance/range analyses, not core fields |
| `bir.hpp: AtomicOperation` and atomic enums; `src/backend/legacy/prealloc/atomics.cpp` | atomic kind/order/result and later target carriers | ordinary atomic instructions with complete operands/results/orderings | carrier/register/loop/helper realization belongs to preparation/MIR |
| `bir.hpp: CallInst`, `CallingConv` | direct/indirect call, args/results, variadic and semantic call flags | symbol-or-operand callee, structural callee type, typed args/results, source convention | `CallArgAbiInfo`, `CallResultAbiInfo`, arg source routing, call moves and homes |
| `src/backend/legacy/prealloc/call_plans.cpp`; `regalloc/call_return_abi.cpp` | ABI classification, source recovery, call placement | core supplies typed call operands, exact producer def-use, symbols and object facts | ABI eligibility is preparation state; chosen abstract homes are E2 BIR products; register/stack realization remains later BIR/MIR work outside Raw/Canonical core |
| `bir.hpp: InlineAsmMetadata`, `InlineAsmOperandMetadata`; `prealloc/inline_asm.cpp` | asm template, constraints, ties, clobbers, memory/address intent and realization | ordinary `InlineAsm` operand/result edges plus original opaque asm/constraint text, ordered clobbers, side effects, and `AsmGotoTerm` CFG | parsed constraints, ties, chosen homes, target operations, and encoding are revision-bound later-stage facts outside Raw/Canonical core |
| `bir.hpp: IntrinsicOperation`; `prealloc/special_carriers.hpp: PreparedIntrinsicCarrier` | intrinsic semantic family plus target carrier | typed semantic intrinsic ID, immediates, operands/results/effects | required target feature legality and carrier placement are pipeline/preparation/MIR |
| `src/backend/legacy/prealloc/variadic*.cpp/.hpp` | entry save areas, va_list layout, va_start/arg/copy/end plans | semantic variadic signature and explicit vararg operations with requested types/layout | register save areas, offsets, helper resources and operand homes |
| `prealloc/dynamic_stack.cpp`, `frame.hpp: PreparedDynamicStackOp` | dynamic alloca/save/restore and final stack realization | `DynamicAlloc`, `StackSave`, `StackRestore`, lifetime operations, typed sizes/alignment | concrete SP adjustment, slots, offsets, alignment sequence |
| `prealloc/special_carriers.hpp`, `f128_runtime_helpers.cpp`, `regalloc/runtime_helpers.cpp` | I128/F128/atomic/intrinsic carriers and helper decisions | exact I128/F128 types/constants and semantic helper-capable operations | lane registers, helper symbol/call ABI, expansion sequence |
| `bir_control_flow_view.cpp`, `bir_route4_publication.cpp`, `bir_route5_publication.cpp` | rediscovered CFG/join/publication identity | stable block/value IDs, terminator successors, phi operands, complete def-use | route indexes and publication/move records; CFG analysis derives predecessors |
| `bir.cpp: make_bir_producer_view`; `bir_route1.cpp`, `bir_route2.cpp`, `query.cpp` | same-block producers, select chains, local/global source lookup | direct `ValueDefinition`, users, typed instruction traversal | named-value scans and cached route records become analyses |
| `bir_route6_call_publication.cpp`, `bir_call_boundary_view.cpp` | call arguments/results and source relationships | descriptors expose complete call inputs/results and exact definitions | publication routing/source-selection plans remain analysis/preparation |
| `bir_route7_comparison.cpp`, `bir_comparison_view.cpp` | comparison producer and branch condition recovery | compare opcode/result IDs plus terminator condition use | fused/materialized target choice is analysis/MIR, not core annotation |
| `bir_route8.cpp`, `bir_return_view.cpp` | return-chain recovery and call-result lanes | optional single return operand, zero-or-one call result, aggregate/complex semantic value, exact def-use | return-chain indexes and every ABI lane home |
| `bir_validate.cpp: validate_*`, `validate` | legacy structural and semantic checks | structured Raw/Canonical verifier rules over stable IDs and descriptors | text-only boolean validation and name fallback |
| `src/backend/legacy/prealloc/liveness.*`, `regalloc/*`, `stack_layout/*` | liveness, homes, allocation, slots, moves, frame | core supplies stable instructions/values/uses/CFG inputs and revisions | intervals/interference are E1 BIR analysis, abstract homes are E2 BIR products, and abstract spill objects plus explicit spill/reload nodes are E3 Pseudo BIR; concrete registers/frame offsets/machine moves remain later |

Coverage means the new IR retains the semantic input needed to recompute the
old outcome. It does not require preserving an old route status, lookup table,
or final target choice as a core field.

## 14. Reference-backend comparison

The required comparison source is `ref/claudes-c-compiler`.

| Reference anchor | Adopt | Reject or strengthen |
|---|---|---|
| `src/ir/module.rs: IrModule`, `IrFunction`, `IrGlobal`, `GlobalInit` | clear module/function/global ownership; rich global initializers; declarations and definitions | public mutable vectors, ABI-specific function fields, string symbol identity |
| `src/ir/instruction.rs: BasicBlock`, `Terminator` | block owns ordered instructions and exactly one terminator; terminator-derived CFG | parallel `source_spans` vector; source attachment belongs on the instruction ID |
| `instruction.rs: BlockId`, `Value`, `Operand` | IDs rather than label strings; explicit value-or-constant operands | bare ownerless `u32`; add epoch/owner/generation and distinct symbol/local namespaces |
| `instruction.rs: Instruction` | broad semantic coverage: allocation, memory, scalar ops, calls, GEP, varargs, atomics, phi, inline asm, intrinsics, select, dynamic lifetime | no `InstId`; direct enum-in-vector mutation; ABI-return lane pseudo-ops are rejected from core in favor of one aggregate/complex semantic value |
| `instruction.rs: Terminator::{IndirectBranch,Switch}` | computed goto and switch successors must be explicit core CFG | possible-target lists must use stable owned `BlockId` and verifier checks |
| `src/ir/analysis.rs: FlatAdj`, `CfgAnalysis` | derive dense disposable CFG from semantic IDs/terminators | dense positions cannot become mutation or persistent identity; bind result to revision |
| `src/passes/mod.rs` | fixed ordered function transformations; reuse an analysis when preservation is proven | comment/manual invalidation; require mutation summaries and checked preservation |
| `src/ir/mem2reg/phi_eliminate.rs` | explicit out-of-SSA boundary, critical-edge splitting, parallel-copy correctness | direct synchronization of vectors/spans/IDs; use editor APIs and produce later-stage MIR data |
| `src/backend/liveness.rs` operand visitors | complete central traversal is essential to backend correctness | hand-maintained consumer-specific visitors; descriptor traversal is canonical and audited |
| `src/ir/intrinsics.rs: IntrinsicOp` | target-independent intrinsic semantics and purity/effect information | ISA opcode lists presented as generic semantics without a feature/legality layer; target selection remains later |

The reference demonstrates the required feature breadth, but c4c deliberately
adds stable instruction/local/symbol identities, transactional mutation,
revision-bound analyses, and strict stage separation.

### 14.1 Producer/source gaps that core must not conceal

Core coverage does not imply that today's `codegen::lir::LirModule` can produce
every form losslessly. The import gate must reject rather than synthesize facts
for these confirmed gaps:

- `LirInst` has no typed atomic load/store/RMW/compare-exchange/fence family;
  the legacy parallel `Function::atomic_operations` table is evidence of needed
  semantics, not an allowed side-table import route.
- `LirInlineAsmOp` now carries ordered ordinary SSA inputs/results with types,
  input/output/read-write roles, and source constraint positions for the
  bounded non-goto transport slice. It still lacks typed symbol/address-space
  value carriers and asm-goto topology; those richer forms must be rejected
  rather than synthesized. Parsed alternatives and symbolic constraint meaning
  are intentionally produced only by the later revision-bound constraint
  stage.
- `LirGlobal::init_text` is still compatibility text and its function-ID list
  is not a recursive initializer tree. It cannot authoritatively produce
  sparse object bytes, arbitrary symbol/block relocations, nested aggregates,
  or label differences without a structured producer addition.
- legacy/compatibility calls can omit full callee signatures, structured
  arguments, `returns_twice`/unwind/memory effects, and typed operand bundles.
  A name and return type do not satisfy `CallPayload`; missing facts are a
  source gap, not defaults inferred from spelling.
- current LIR has no top-level-asm carrier, including no producer-supplied
  ordered `SymbolId` dependency vector; it also lacks alias, symbol-version,
  constructor-priority, destructor-priority, and handled exception/unwind
  families. Core has owners for the former module directives, but import must
  report the missing carrier and must never recover dependencies by parsing asm
  text. V1 deliberately admits only `MayUnwind` escape effects; invoke,
  cleanup-pad, landing-pad, and local unwind-edge forms fail import.
- the target schema distinguishes IEEE binary16/32/64/128 from x87
  extended-80 semantics and keeps semantic width separate from object storage
  size/alignment. Current LIR text that says only `F128`, `half`, or `long
  double` without the resolved `FloatFormat` and layout cannot choose among
  those alternatives; import must reject that ambiguity until the producer
  carries the structured format.

These are producer-boundary blockers for the affected source forms, not reasons
to add `Unsupported` opcodes, opaque text authority, testcase exceptions, or
legacy prepared records to Raw BIR.

## 15. Auditable backend semantic coverage status

This table audits design ownership, not implementation or end-to-end compiler
completion. `schema-owned` means this document assigns a closed core schema;
`source-gap` means the core schema exists but today's LIR cannot populate it
losslessly. No acceptance-critical v1 family remains architecturally open.

| Feature family | Status | Evidence / remaining boundary |
|---|---|---|
| Integer, pointer, aggregate, vector, function and exact constant types | `schema-owned` | `TypeData`, `ConstantData`, `SemanticDataLayout`; ABI classes remain later |
| IEEE/x87 semantic format versus storage layout | `source-gap` | `FloatFormat` and `FloatLayout` are separate, but current LIR text can be ambiguous |
| Symbols, declarations/definitions, linkage, TLS, aliases and object layout | `schema-owned` | `SymbolData`, `GlobalData`, `LocalId`; no frame or target address policy |
| Recursive global initializers, relocations, block addresses and label differences | `source-gap` | sparse initializer schema exists; `LirGlobal::init_text` is not authoritative input |
| Scalar, aggregate/vector, memory/GEP and runtime-helper-capable semantics | `schema-owned` | closed opcodes/descriptors retain semantics; helper and target selection remain later |
| Dynamic allocation, stack save/restore, lifetime and explicit vararg operations | `schema-owned` | semantic operands/effects only; frame and va-list ABI plans remain later |
| Atomic load/store/RMW/cmpxchg/fence | `source-gap` | closed core payload exists; current `LirInst` has no typed atomic family |
| Direct/indirect calls, zero-or-one result, effects, attributes and bundles | `source-gap` | `CallPayload`, `CallEffects`, `CallSiteAttributes`; compatibility calls omit required facts |
| CFG, exact parallel-edge `EdgeKey` phi authority and single-value return | `schema-owned` | terminator-derived successor-slot multiset; no physical return lanes |
| Inline asm constraints, operands, clobbers and asm-goto edges | `bounded-current` plus `source-gap` | current generic SSA inputs/results and original opaque payload transport non-goto asm; typed symbol/address-space value carriers and asm-goto topology remain source gaps, while parsed constraints are intentionally later revision-bound facts |
| Top-level asm, symbol versions and constructor/destructor priority | `source-gap` | ordered module records exist; current LIR has no complete producer family |
| Target-independent intrinsic namespace | `schema-owned` | core owns the build-versioned semantic registry and P07 canonicalizes aliases; ISA selection and feature support remain later |
| Modern asm-goto output-edge value semantics | `schema-owned` plus `source-gap` | paired `InlineAsm` results are ordinary definitions available on every successor; edge-specific output definitions are rejected by v1 |
| Debug scopes/types required for object debug output | `schema-owned` | file/scope/location IDs and origin attachments are the retained subset; semantic `TypeId` supplies object type/layout and no parallel debug-type authority exists |
| Exception/unwind/cleanup-pad control flow | `source-gap` | `MayUnwind` models escape only; v1 rejects local unwind destinations and stores no hidden exception edges |
| Stable IDs, reservation, exact def-use, transactions, revisions and atomic publication | `schema-owned` | `ReservedInst`, descriptors, editors, `ModuleDraft -> verify -> RawBir` |
| ABI placement, chosen homes, spill/reload, call moves, frame, opcodes and emission | `schema-owned` | excluded from Raw/Canonical core; D2 prepares ABI/call facts, E1 owns allocation liveness, E2 owns abstract homes, E3 owns explicit Pseudo BIR spill/reload, E4 publishes, and MIR owns concrete target realization |

## 16. Closed v1 policy choices

The remaining policy is fail-closed and does not defer architecture decisions:

1. Source attributes not named by the closed function/call registries are
   rejected when semantically required; diagnostics-only attributes may be
   dropped. New semantic attributes require an explicit registry extension and
   stage owner.
2. Interned `TypeId` and `ConstantId` slots are append-only for one module epoch;
   they tombstone but are not generation-reused. Other slot maps retain the
   generation-reuse rules in section 4.
3. Raw publication requires resolved structured object size/alignment and exact
   floating format/storage layout. Unresolved layout tokens are a producer gap,
   never temporary Raw payload.
4. Architecture acceptance does not imply implementation completion. The
   bootstrap source gaps catalogued above remain fail-closed until their typed
   carriers and verifier rules are implemented.
