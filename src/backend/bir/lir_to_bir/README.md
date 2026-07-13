# LIR-to-Raw-BIR Import Design

Status: `under-review`. This is a design contract, not a statement that the
current importer implements it.

The importer is the only stage allowed to translate `codegen::lir::LirModule`
identity and semantics into new-BIR identity. Its job is lossless semantic
transport into a verified `RawBir`; it is not the first optimization pass and
it is not a target-lowering stage.

## 1. Stage boundary and authority

```text
verified LirModule
  -> validate and inventory source surface
  -> predeclare types, symbols, functions, blocks, and value identities
  -> import globals and function bodies
  -> resolve deferred operands and CFG edges
  -> finish one ModuleDraft
  -> atomically verify-and-publish that revision as RawBir
```

Input authority:

- `LirModule` owns source ordering, `LinkNameId`/`StructNameId`, target/data
  layout selection, globals, string constants, function signatures, bodies,
  stack objects, and intrinsic requirement metadata.
- Structured fields are authoritative when present. Rendered type, operand,
  signature, initializer, or symbol text is a compatibility input only.
- The importer may diagnose disagreement between structured and rendered
  mirrors. It must never silently choose the rendered mirror over valid
  structured identity.

Output authority:

- A successful call returns one move-only, Raw-profile-verified `RawBir`.
- BIR IDs are the only published identity. LIR IDs and spellings may survive as
  origin/debug metadata but not as lookup authority.
- Terminators are the only CFG successor authority. Predecessors, use lists,
  dominance, liveness, provenance closures, and call graphs are recomputable
  analyses, not importer-owned side tables.
- Raw instructions preserve source semantics even when their shape is not yet
  canonical. The first named canonical pass owns each permitted Raw-only form.

Source-truth rule:

- this contract distinguishes **present structured fact**, **audited
  compatibility text**, and **producer gap**. A required backend fact seen in
  legacy BIR or the reference compiler is not thereby present in today's LIR;
  the importer must not fabricate it;
- `extern_decl_*_map`, `struct_decl_index`, `str_pool_map`, counters, intern
  storage, and layout-observation caches are producer indexes/audit state, not
  additional semantic module members to publish. Their contents must agree with
  the ordered semantic vectors/tables when validation can compare them;
- `target_profile` and `data_layout` are interpretation context. They may be
  copied into module context and used to validate type/object facts, but do not
  authorize ABI classification or target instruction selection.

The importer must not decide ABI register/stack placement, call moves, frame
layout, register allocation, spill/reload, instruction selection, target
opcodes, or instruction encodings. Target/data-layout facts may be preserved
when they are part of source semantics (for example pointer width, explicit
alignment, address space, or an already-classified calling convention), but
they must not be converted into physical placement decisions here.

This prohibition is permanent for Raw/Canonical BIR. Later typed plans or MIR
may own homes, spills, reloads, frame facts, and target operations, but the
importer neither creates nor predicts those facts and no later stage writes
them back into Raw/Canonical storage.

### 1.1 Current `LirModule` inventory

Every current top-level field has this disposition:

| Source field family | Import disposition |
|---|---|
| `target_profile`, `data_layout` | copied as module interpretation context; validated, never converted to placement |
| `link_name_texts`, `link_names`, `struct_names`, `type_tag_storage` | resolve/copy stable identities and required display text; storage addresses never escape |
| `globals`, `functions`, `string_pool`, `extern_decls` | ordered semantic inputs, predeclared before definitions/initializers |
| `extern_decl_link_name_map`, `extern_decl_name_map` | producer dedup indexes; parity-check against ordered declarations, never import as a second declaration set |
| `type_decls`, `struct_decls`, `struct_decl_index` | structured declarations win; text is audited shadow; index is parity-only |
| `structured_layout_observations` | validation/audit evidence, not type or layout authority |
| `need_va_*`, `need_mem*`, `need_stack*`, `need_abs`, `need_ptrmask`, `prefer_semantic_va_ops` | consistency-check against actual semantic operations/required declarations; do not synthesize missing instructions from flags |
| `spec_entries` | preserve ordered specialization/link identity metadata if core gains a typed record; otherwise diagnose unsupported module metadata |
| `str_pool_map`, `str_pool_idx` | producer cache/counter; parity-check names/content/order, never use hash order for BIR IDs |

Adding a semantic `LirModule` field requires adding a row and import-or-reject
handling. C++ reflection cannot enforce this automatically, so source review
and a maintained inventory test accompany the closed instruction visitors.

## 2. Proposed public API

The public surface should replace the empty bootstrap `ImportOptions` without
re-exporting importer implementation maps:

```cpp
namespace c4c::backend::bir {

enum class CompatibilityTextCase : std::uint8_t {
  TypeMirrorParity,
  OpcodeMirrorParity,
  LinkNameMirrorParity,
  DebugNameMirror,
  Count,
};

struct CompatibilityWhitelist {
  std::bitset<static_cast<std::size_t>(CompatibilityTextCase::Count)> enabled;
  [[nodiscard]] bool permits(CompatibilityTextCase) const;
  static CompatibilityWhitelist production() { return {}; } // fail closed
};

enum class DiagnosticLevel : std::uint8_t { Note, Warning, Error };

struct ImportOptions {
  CompatibilityWhitelist compatibility = CompatibilityWhitelist::production();
  bool verify_lir = true;
  bool preserve_origin_metadata = true;
  bool warnings_are_errors = false;
};

enum class ImportPhase : std::uint8_t {
  ValidateInput,
  PredeclareTypes,
  PredeclareSymbols,
  ImportInitializers,
  PredeclareFunctionBody,
  ImportInstructions,
  ResolveFixups,
  FinishDraft,
  VerifyAndPublish,
};

enum class ImportErrorCode : std::uint16_t {
  MalformedLir,
  DuplicateIdentity,
  ConflictingDeclaration,
  MissingType,
  TypeMismatch,
  MissingSymbol,
  MissingBlock,
  MissingValue,
  DuplicateValueDefinition,
  InvalidForwardReference,
  InvalidPhiIncoming,
  InvalidTerminator,
  InvalidInitializer,
  UnsupportedSemanticFamily,
  UnsupportedCompatibilityText,
  BuilderFailure,
  VerificationFailure,
  ResourceExhausted,
};

struct ImportSite {
  std::optional<LinkNameId> function;
  std::optional<codegen::lir::LirBlockId> block;
  std::optional<std::size_t> instruction_index;
  std::optional<codegen::lir::LirValueId> value;
  std::string field;
};

struct ImportDiagnostic {
  DiagnosticLevel level = DiagnosticLevel::Error;
  ImportPhase phase = ImportPhase::ValidateInput;
  ImportErrorCode code = ImportErrorCode::MalformedLir;
  ImportSite site;
  std::string message;
};

struct ImportReport {
  std::vector<ImportDiagnostic> diagnostics;
  std::size_t type_count = 0;
  std::size_t global_count = 0;
  std::size_t function_count = 0;
  std::size_t block_count = 0;
  std::size_t instruction_count = 0;
  std::size_t compatibility_use_count_total = 0;
  std::array<std::size_t,
             static_cast<std::size_t>(CompatibilityTextCase::Count)>
      compatibility_use_count{};
};

struct ImportedRawBir {
  RawBir module;
  ImportReport report;
};

using ImportFailureCause = std::variant<BirError, PublicationFailure>;

struct ImportFailure {
  ImportReport report;
  std::optional<ImportFailureCause> cause;
};

Result<ImportedRawBir, ImportFailure> lower_lir_to_raw_bir(
    const codegen::lir::LirModule&, ImportOptions = {});

}  // namespace c4c::backend::bir
```

The whitelist is per case, never a global “allow text” switch:

| Case | Preconditions | Permitted action | Never permitted |
|---|---|---|---|
| `TypeMirrorParity` | complete structured `TypeId`/shape already resolved | parse/render only to compare the mirror | create a field, union kind, address space, function component, or layout |
| `OpcodeMirrorParity` | typed opcode/predicate enum already present | compare canonical spelling | turn an unknown spelling or legacy numeric opcode into semantics |
| `LinkNameMirrorParity` | valid `LinkNameId`/`SymbolId` already selected | compare copied link spelling | resolve a missing direct callee/global/relocation identity by name |
| `DebugNameMirror` | structured entity ID/topology already selected | copy a display/debug name | resolve SSA uses, blocks, phi edges, or control-flow targets |

Every permitted use increments both the total and exact-case counter and emits
an origin-linked note; disagreement is an error. Production enables no cases.
Migration tooling may enable named cases individually. There is deliberately no
case for `init_text`, `signature_text`, call `args_str`/callee text, GEP index
text, phi/terminator labels, inline-asm constraints/arguments, or blockaddress
text: those strings may not create missing identity, topology, types,
signatures, operands, effects, or relocations. Inline-asm template bytes are
source payload, not a compatibility fallback, but still require structured
operand/control metadata alongside them.

There is deliberately no public API that returns a partially built module.
Callers may inspect diagnostics on failure, but not incomplete BIR storage.
The convenience API must not throw for an expected unsupported LIR family.
`BirError` is core-owned and `PublicationFailure` is verifier-owned; this file
only references those single definitions and does not redeclare, flatten, or
translate their rule taxonomies. Builder/finish failure stores `BirError` in
`cause`; publication failure stores the verifier's unchanged
`PublicationFailure`, including its complete `VerificationReport` and optional
preceding `BirError`. Pure source validation failures may have no lower-layer
cause because `ImportReport` already identifies them precisely.

## 3. Internal API and `ImportContext`

The implementation should be phase-oriented, not one monolithic variant
visitor. Proposed internal interfaces:

```cpp
class ImportContext {
 public:
  static Result<ImportContext, ImportFailure> create(
      const codegen::lir::LirModule&, ImportOptions);

  Result<void, ImportFailure> validate_source();
  Result<void, ImportFailure> predeclare_types();
  Result<void, ImportFailure> predeclare_symbols();
  Result<void, ImportFailure> import_global_initializers();
  Result<void, ImportFailure> import_functions();
  Result<void, ImportFailure> resolve_module_fixups();
  Result<ImportedRawBir, ImportFailure> publish() &&;

  Result<TypeId, ImportFailure> require_type(
      const codegen::lir::LirTypeRef&, ImportSite);
  Result<TypeId, ImportFailure> require_type(
      const TypeSpec&, ImportSite);
  Result<SymbolId, ImportFailure> require_symbol(LinkNameId, SymbolKind,
                                                 ImportSite);
  Result<FunctionId, ImportFailure> require_function_definition(
      LinkNameId, ImportSite);
  Result<GlobalId, ImportFailure> require_global_definition(
      LinkNameId, ImportSite);
  void diagnose(ImportDiagnostic);

 private:
  ImportContext(const codegen::lir::LirModule&, ImportOptions,
                ModuleBuilder&&);

  const codegen::lir::LirModule& source_;
  ImportOptions options_;
  ModuleBuilder builder_;
  ImportReport report_;
  TypeImportTable types_;
  SymbolImportTable symbols_;
  std::vector<ModuleFixup> module_fixups_;
};

class FunctionImportContext {
 public:
  FunctionImportContext(ImportContext&, FunctionBuilder&,
                        const codegen::lir::LirFunction&);

  Result<void, ImportFailure> predeclare_blocks();
  Result<void, ImportFailure> reserve_instructions_and_values();
  Result<void, ImportFailure> define_non_phi_instructions();
  Result<void, ImportFailure> import_terminators();
  Result<void, ImportFailure> resolve_and_define_phis();
  Result<void, ImportFailure> resolve_remaining_fixups();

  Result<Operand, ImportFailure> operand(codegen::lir::LirValueId,
                                         TypeId expected, OperandRole,
                                         ImportSite);
  Result<BlockId, ImportFailure> block(codegen::lir::LirBlockId, ImportSite);
  Result<void, ImportFailure> copy_debug_name(
      std::string_view, BlockId already_resolved, ImportSite);
  Result<BuildResult, ImportFailure> append(BlockId, InstSpec, ImportSite);
};
```

`ImportContext::create` stores failure from `ModuleBuilder::create` as the
`BirError` alternative of `ImportFailure::cause`. There is intentionally no option to disable Raw
verification: tests that need malformed draft state test builders/verifier
directly and never receive a `RawBir`. Production should keep `verify_lir`
enabled; disabling that preflight for importer-focused tests does not disable
the importer's own closed dispatch validation or publication verification.

`FunctionImportContext` exists only inside one
`ModuleBuilder::with_function` callback. It cannot retain or expose a
`FunctionBuilder` after that capability expires. Importer-local decoding data
is not a second builder schema: dispatch ultimately produces the core
`InstSpec`, whose opcode descriptor must expose every operand/result slot to
def-use and verification.

Internal tables are typed by source identity:

- `StructNameId -> TypeId` plus a recursion state for opaque/recursive types.
- `LinkNameId -> SymbolId`; the symbol then owns/refers to its `FunctionId` or
  `GlobalId` declaration/definition. A direct `CallPayload` uses `SymbolId`, as
  drafted by core, so an extern declaration never needs a fabricated
  `FunctionId`. Definition IDs are requested only when importing a body or
  object definition.

This document deliberately resolves the adjacent core review question in favor
of `SymbolId` for a direct callee: extern calls have link-visible identity even
when no body-owning `FunctionId` exists. A function symbol still carries a
structural function type and may reference its declaration/definition record;
the importer rejects a non-function `SymbolKind` at the call site.
- `LirBlockId -> BlockId`; label text may be copied only as `DebugNameMirror`
  after the ID is already selected and never participates in lookup.
- `LirValueId -> Operand` for structured-ID alternatives. Ordinary instruction
  results map to `ValueId`; constant-result stubs map directly to interned
  `ConstantId`. Global/function references arrive as IDs and become `SymbolId`;
  local storage arrives as `LirStackSlotId` and becomes `LocalId`-based
  operations. There is no `string -> Operand` or `string -> BlockId` semantic
  table. `LirOperand::kind()` is only lexical classification; it cannot resolve
  an operand.
- `LirStackSlotId -> LocalId` (`LocalId` is semantic storage, never a frame
  index or offset).
- source instruction coordinate -> move-only `ReservedInst` until definition;
  afterward retain only its `InstId` for diagnostics/origin.

No internal table, raw source pointer, `string_view`, iterator, vector index, or
builder capability may escape publication. Origin metadata, if enabled, must
copy stable source coordinates rather than retaining references into LIR.

## 4. Ordered module construction

### Phase 0: source validation and inventory

Run the LIR verifier before allocating BIR IDs. Inventory every module field
and every `LirInst` alternative. Validation checks table ownership, ID validity,
unique definitions, function/block structure, structured/text mirror parity,
terminators, and rejection of initializer syntax represented only as text.
Unknown variant alternatives are hard errors, not ignored extensions. New C++
data members cannot be discovered by a runtime visitor, so exhaustiveness is a
compile-time maintenance obligation: a closed `std::visit` with no generic
success arm plus an inventory test/static count must fail review/build when
`LirInst` or `LirTerminator` changes. Producer-only maps/caches are parity
checked where applicable and otherwise ignored as non-semantic state.

### Phase 1: type predeclaration

Predeclare named struct/union/opaque identities before defining their bodies so
recursive pointer types can resolve. Then define fields in source declaration
order and preserve packed/opaque state. Import scalar integers including i1,
i8/i16/i32/i64/i128, f16/f32/f64/f80/f128 where source can represent them,
pointers with address space, arrays, vectors, functions, and aggregate types.

`TypeSpec`, `LirTypeRef`, and `StructNameId` must converge on one BIR `TypeId`.
If structured facts disagree with `type_decls` text, report the disagreement;
the legacy text must not redefine the structured type. Size/alignment are
validated against the selected data layout but do not become frame decisions.

Today's `LirTypeRef` structurally classifies kind, integer width, VRM width, and
optional `StructNameId`, but most compound shape remains rendered text;
`LirStructDecl` supplies fields/packed/opaque but does not distinguish struct
from union. Pointer address space and function-type components are likewise not
structured. Text may check parity only after a complete structured type exists;
otherwise these are producer gaps, not facts the importer may infer from
downstream target behavior.

### Phase 2: symbol and storage predeclaration

Predeclare in deterministic source order:

1. string-pool objects;
2. globals (definitions and extern declarations);
3. explicit extern function declarations;
4. function declarations and definitions;
5. specialization/linkage metadata that affects symbol identity.

Merge repeat declarations only when link identity, kind, type, linkage,
visibility, TLS/constant state, and calling convention agree. Function and
global addresses in initializers can then resolve regardless of declaration
order. Missing link identity is an error. An enabled `LinkNameMirrorParity`
case may compare spelling only after `LinkNameId`/`SymbolId` selection, and its
use is counted and diagnosed.

Current module metadata is materially narrower than this target. `LirGlobal`
has structured internal/const/extern/alignment flags, but linkage/visibility is
a combined rendered `linkage_vis` string and there are no structured fields for
thread-local storage, TLS model, common/weak/linkonce, section, visibility,
`used`, alias targets, constructors/destructors, symbol versions, or top-level
assembly. The reference `IrModule` has many of these, proving downstream
backend need but not current LIR availability. They are explicit producer gaps;
the importer must neither default them from spelling nor silently drop them.

Top-level assembly additionally requires a typed dependency surface. The target
`TopLevelAsmRecord` must carry, alongside source text/origin, an ordered
`std::vector<SymbolId> symbol_dependencies` naming every predeclared
link-visible symbol whose definition/reference affects retention or ordering.
Scanning assembly text to discover names is forbidden. Current LIR has neither
a top-level-asm record nor this dependency list, so it is a producer gap; the
core schema must also settle whether dependency roles (definition versus use)
need a closed typed discriminant before implementation.

Computed-goto execution is partly present as `LirIndirectBrOp`, but taking a
label address is emitted today as raw `blockaddress(...)` operand text, including
inside `init_text`. There is no structured `(FunctionId, BlockId)` source
carrier. Compatibility text cannot create that identity/topology; full coverage
requires a typed block-address operand/constant and typed label-difference
initializer.

`LirStringConst::raw_bytes` is currently populated with LLVM-escaped rendering
despite its name, plus a byte length; it is not an authoritative byte vector.
Embedded NUL, wide/UTF-16 element width, and exact terminator policy therefore
require a structured byte/element producer carrier. The compatibility
whitelist cannot decode rendered bytes into missing source data.

### Phase 3: global initializers

Import initializers only after all target symbols exist. The semantic model must
cover:

- zero, undef/poison where legal, integer, floating, pointer, and null values;
- byte strings including embedded NUL and explicit storage length;
- arrays, structs, unions, padding bytes, nested aggregates, and repeated zero;
- symbolic addresses of globals, functions, strings, and label-address forms
  where the language permits them;
- addends/GEP projections, pointer-width integer casts, and relocation slots;
- tentative/common, extern, internal, weak/linkonce-like linkage, const,
  explicit alignment, TLS, and declaration-without-initializer state.

Relocations are typed `(target identity, addend, width, relocation semantic)`,
not parsed symbol spellings stored as authority. No text parser exists at this
boundary; the LIR producer must supply a structured initializer tree.
Initializer failure rejects the whole module.

This is a required target model, not current-source capability. Current
`LirGlobal` has `init_text` and only a side vector of referenced function
`LinkNameId`s; it has no structured initializer tree, relocation slots,
symbol-plus-addend expression, label difference, padding node, TLS relocation,
or nested typed aggregate identity. `initializer_function_link_name_ids` can
validate/resolve function references but cannot reconstruct initializer
topology. No compatibility case permits parsing `init_text` into missing
semantics; import fails until the producer supplies the structured tree.

### Phase 4: function skeletons

For every function, create its signature and attributes before importing any
body. Preserve parameter and result types, variadic/unspecified/void parameter
distinctions, linkage, calling convention, by-value/sret semantic attributes,
extension attributes, and declaration/definition state. ABI classification or
physical locations are deferred to preparation.

Current LIR structurally carries variadicness, void-list state, byval on
signature parameters, and return extension on call/declaration records. It does
not provide a general function calling-convention field, sret attribute,
parameter extension attributes, unspecified-parameter flag on definitions, or
complete function attributes. Missing facts are producer gaps; text in
`signature_text` is compatibility-only.

For a definition, create blocks using `LirBlockId` order and record the explicit
entry ID; never infer entry solely from vector position. Predeclare stack
objects and hoisted/inline allocas as semantic storage operations with element
type, count, alignment, volatility/address-taken semantics, and static versus
dynamic lifetime. Do not assign frame offsets.

Current `LirStackObject` supplies type/alignment/`is_vla`; `LirAllocaOp`
supplies rendered element type, optional count, and alignment. Neither supplies
the full volatility/address-taken/lifetime metadata named above. Preserve what
is present and record the rest as producer gaps rather than guessing from use
patterns during import.

### Phase 5: value and instruction import

Scan parameters and all result-bearing instructions first. Intern constant
definitions and reserve one typed `ValueId` for each runtime instruction result;
reject duplicate source definitions before emission. This makes loop-carried
phi operands and other permitted forward references independent of source
visitation order without inventing constant-producing runtime instructions.

Use the core reservation API exactly:

```cpp
class ReservedInst {
 public:
  ReservedInst(ReservedInst&&) noexcept;
  ReservedInst& operator=(ReservedInst&&) noexcept;
  ReservedInst(const ReservedInst&) = delete;
  ReservedInst& operator=(const ReservedInst&) = delete;
  InstId instruction() const;
  std::span<const ValueId> results() const;
};
class FunctionBuilder {
 public:
  BirResult<ReservedInst> reserve_instruction(
      BlockId, InstPosition, std::span<const TypeId> result_types);
  BirResult<BuildResult> define_reserved_instruction(
      ReservedInst&&, InstSpec);
  // Other core methods omitted here.
};
```

Reservation immediately fixes `InstId`, result `ValueId`s, and instruction
order. Definition must match the reserved result arity/types exactly. Each
move-only token is consumed once; any undefined token/reservation blocks
`finish()` and therefore publication.

Reserve all instructions in source order, define non-phi instructions, import
terminators, then define reserved phis after exact `EdgeKey` resolution. Each
defined instruction records exact operand roles and result types. No folding,
branch-chain following, select-chain recognition, memcpy scalarization,
aggregate leaf-slot expansion, comparison rewriting, or immediate arithmetic
evaluation is allowed in this phase: those are canonical pass work.

### Phase 6: fixups and terminators

Import one terminator per block after resolving every target. Preserve switch
case values and order deterministically, reject duplicate conflicting cases,
and preserve the complete possible-target set for indirect branches. Compute no
persistent predecessor/successor cache.

Then derive every terminator `SuccessorSlot` and its exact
`EdgeKey{source, role, index}`. Resolve phi fixups against the incoming edge-key
**multiset**, requiring exactly one typed incoming operand for every exact
`EdgeKey` and no extra key. Predecessor-block cardinality is never the rule: two
switch cases, asm-goto slots, or other parallel edges from one source block to
the same destination remain distinct. Today's `LirPhiOp` carries only
`(value-text, predecessor-label)`; it may map only when structured value/block
identity exists and that predecessor has exactly one matching successor slot.
Parallel matches are ambiguous and fail until LIR carries structured
`(EdgeKey, operand)` phi entries.

### Phase 7: verification and atomic publication

`ImportContext::publish() &&` consumes its `ModuleBuilder` exactly once through
`finish() &&` to obtain one private `ModuleDraft`, then makes exactly one call:

```cpp
Result<RawBir, PublicationFailure> published =
    verify_and_publish_raw(std::move(draft));
```

That gate freezes, fully Raw-verifies, and consumes the same draft revision
atomically. Success alone mints the unforgeable move-only `RawBir`; failure
returns `PublicationFailure` and no usable draft/stage object. Public
`verify_candidate` is diagnostic-only and is never invoked first to manufacture
or cache a publication proof. The importer wraps builder/finish failure in
the `BirError` cause and the gate failure unchanged as the
`PublicationFailure` cause, while
retaining source-site diagnostics in `ImportReport`. It never constructs or
returns an unverified `RawBir`.

## 5. Exhaustive `LirInst` dispatch contract

Every alternative currently listed in `codegen/lir/ir.hpp::LirInst` must have a
named row. “Legacy” below means an older structured-ID variant, not permission
to ignore it. A row describes the target mapping; if a current field supplies a
required operand/type/identity only through `LirOperand`, `LirTypeRef`, or other
text without an already-complete structured counterpart, the closed whitelist
rules above make that current instance an explicit source-gap failure.

| LIR alternatives | Raw-BIR semantic form | Required preserved facts | First later owner |
|---|---|---|---|
| `LirConstInt`, `LirConstFloat` | intern core `ConstantId` and bind the source result to that `Operand`; emit no runtime instruction | current fields are `long long`/`double`; exact i128 and f80/f128 constants are a **source gap**, not recoverable bits | `legalize` validates representable constants; producer must add arbitrary-width/bit-pattern constants |
| `LirLoad`, `LirLoadOp` | core `Load` | pointer and value type; volatile, explicit alignment, and address space are **source gaps** in both current forms | `memory` canonicalizes address/effects after producer gaps close |
| `LirStore`, `LirStoreOp` | core `Store` | pointer and stored value/type; volatile, alignment, and address space are **source gaps** | `memory` |
| `LirBinary`, `LirBinOp` | core `Binary`/`Unary(FNeg)` | new form must have a typed opcode; old `int op` is a **source gap** and fails because compatibility cannot create opcode semantics; preserve operands and future flags | `scalar` |
| `LirCast`, `LirCastOp` | core `Cast` | new form has exact cast kind/from/to type; old form has no cast kind and fails when the kind is not uniquely determined | `legalize`, then `scalar` |
| `LirCmp`, `LirCmpOp` | core `Compare` | new form must have domain/type/full typed predicate; old integer predicate is a **source gap** and fails when those facts are absent | `scalar` |
| `LirCall`, `LirCallOp` | core `Call` | old form has name/pointer plus untyped value IDs; new form is accepted only with structured direct identity or callee operand, signature, structured args, varargs boundary, and attributes; call text never fills a missing fact | `legalize` validates the already-typed call; ABI `preparation/calls` later |
| `LirGep`, `LirGepOp` | core `GetElementPtr` | old indices are value IDs; new indices are `"type value"` strings; without structured typed indices the form is a **source gap** and fails | `memory` normalizes an already-typed path after the producer replaces index text |
| `LirSelect`, `LirSelectOp` | core `Select` | condition and both typed values; no compare fusion | `scalar` |
| `LirIntrinsic` | core `Intrinsic` only after lossless registry recognition | current form has only name, optional result ID, and argument IDs: result type, semantic ID, effects, immediates, and feature contract are **source gaps** | `intrinsics`; unknown/underspecified names fail |
| `LirInlineAsm`, `LirInlineAsmOp` | core `InlineAsm` only with a complete structured payload | current forms preserve template/constraints and the new form clobbers/side-effects/`insn_r`, but operands/types/ties/names/goto labels remain absent or buried in `args_str`; missing structure fails import | `legalize` validates already-structured constraints; `preparation/inline_asm` later chooses placement |
| `LirMemcpyOp`, `LirMemsetOp` | semantic memory intrinsic | operands, byte count/value, volatility, align/address spaces if present | `memory` is the first owner; `intrinsics` may later canonicalize registry identity |
| `LirVaStartOp`, `LirVaEndOp`, `LirVaCopyOp`, `LirVaArgOp` | semantic variadic operations | va-list address(es), result type, aggregate shape if present | `intrinsics`; variadic preparation later |
| `LirStackSaveOp`, `LirStackRestoreOp` | semantic dynamic-stack lifetime operations | saved/restored pointer identity and ordering | `memory`; frame realization later |
| `LirAbsOp` | semantic absolute-value intrinsic | integer type, source, result, poison/overflow contract if source adds it | `intrinsics` |
| `LirIndirectBrOp` | target mapping is core `IndirectJumpTerm`, never an ordinary instruction | current producer emits it last with default `LirUnreachable`, but its address/targets are text-only; this is a **source gap** until structured operand/`BlockId`s exist | with structured fields, importer consumes it, requires it last, reconciles only the documented sentinel, and establishes sole terminator authority; current text form fails |
| `LirExtractValueOp`, `LirInsertValueOp` | raw aggregate projection/update | aggregate/element types, index path, undef/poison input | `aggregate` |
| `LirInsertElementOp`, `LirExtractElementOp`, `LirShuffleVectorOp` | raw vector operations | vector/element/index/mask and poison/undef lane semantics must be structured; current text-only missing facts are a **source gap** | `legalize` validates typed shape; structural vector ops remain canonical, target SIMD intrinsics go to `intrinsics` |
| `LirAllocaOp` | raw dynamic/static alloca | element type, count, alignment, lifetime class | `memory`; never frame layout here |

`LirPhiOp` is handled by the phi predeclaration/fixup path, not ordinary
instruction dispatch. Its target carrier preserves result type and every
`(EdgeKey, Operand)` without lowering to moves; the current text/block-pair
shape is a source gap under the rules above. Hoisted
`LirFunction::alloca_insts` uses the
same alloca dispatcher and semantic ordering rules as inline alloca.

The duplicate old/new LIR variants must either map to exactly the same semantic
builder operation or fail with a diagnostic explaining which required fact is
absent. Variant shape must not create two BIR instruction families.

The table covers all 38 alternatives in the current `LirInst` declaration.
That count is an audit tripwire, not an ABI: the implementation's closed visitor
and its inventory test must be updated together whenever the source variant
changes.

## 6. Terminators and CFG

| LIR terminator | Raw-BIR form | Import checks |
|---|---|---|
| `LirBr` | `JumpTerm` | target resolves uniquely |
| `LirCondBr` | `CondJumpTerm` | i1/boolean condition; both targets resolve; equal targets remain representable until CFG pass |
| `LirRet` | `ReturnTerm` | zero/one value agrees with semantic function result; aggregate lanes remain semantic values, not return registers |
| `LirSwitch` | core `SwitchTerm` | selector type/value, default, all cases and targets preserved |
| `LirIndirectBr` | core `IndirectJumpTerm` | address plus conservative complete target set |
| `LirUnreachable` | `UnreachableTerm` | no fallthrough inferred |

The importer does not normalize entry blocks, split critical edges, delete
unreachable blocks, thread branches, create fallthrough, or lower switch to a
chain/jump table. Those are CFG or later target decisions.

Most current terminators name targets/conditions/returns with strings;
`LirIndirectBr` alone uses structured IDs, and the current printer/producer
instead uses `LirIndirectBrOp`. Text cannot create CFG/value identity, so those
terminators are producer gaps and fail until structured IDs/operands exist. If
both structured indirect encodings occur, they must agree
exactly; an `LirIndirectBrOp` followed by the producer's default
`LirUnreachable` sentinel is the only documented carrier-to-terminator
conversion, not two sequential terminators.

## 7. Calls, function pointers, and variadics

Direct calls require `direct_callee_link_name_id`; absence is a source error,
not permission to resolve raw symbol text. Indirect calls require a structured
callee operand and preserve its semantic function type; they are never guessed
from spelling. Both forms preserve result type, return/parameter extension
attributes, fixed argument count, variadic marker, by-value/sret semantic
attributes, aggregate type/layout identity, and source argument order.

Those are acceptance requirements, not a claim about every current variant.
`LirCall` has no structured callee signature or per-argument types;
`LirCallOp` has optional signature/structured arguments but may contain only
`callee_type_suffix`, `args_str`, and type-fragment mirrors. An indirect call
without a structurally recoverable function type, or a variadic call without a
recoverable fixed/variadic boundary, fails rather than borrowing ABI facts from
legacy lowering.

Likewise, `LirExternDecl` currently carries only return type/extension and link
identity, not a complete parameter list or variadic/calling-convention
signature. Call-site observations may be checked for agreement but cannot
silently become a canonical declaration when incompatible call sites exist.

### 7.1 Exhaustive call-site field disposition

Every core call field is required input, not a default chosen by import:

| Core field | Current LIR disposition |
|---|---|
| direct callee operand | preserve `direct_callee_link_name_id` as `SymbolId`; missing direct identity fails |
| indirect callee operand | requires a structured typed operand; current text-only `callee` is a producer gap |
| `CallPayload::callee_type` | preserve a complete structured `callee_signature`; absent/incomplete signature fails |
| `source_calling_convention: CallingConvention` | no current call-site field: producer gap; never default to `C` |
| `fixed_argument_count` | derive only from complete structured fixed parameters and check actual arguments; otherwise fail |
| `parameter_list_kind: ParameterListKind` | map structured `Prototype`/`Variadic`/`Unspecified` state exactly; conflicting or absent state fails |
| ordinary argument operands/types | preserve ordered structured operand IDs and `TypeId`s; current `LirOperand`/`args_str` values are insufficient |
| zero-or-one semantic result and type | preserve structured result identity/type; text result spelling cannot define it |
| `CallSiteAttributes::tail_request: TailRequest` | no current field: producer gap; never default to `None` |
| `argument_attributes[].extension` | preserve each structured `LirCallArg::ext_attr` when the corresponding structured argument exists |
| argument `by_value`, `structure_return_pointer`, `by_value_layout` | no complete current call-site fields: producer gap; never infer from rendered types or ABI observations |
| `return_attributes.extension` | preserve `return_ext_attr` and require agreement with the structured signature/declaration |
| `CallEffects::return_behavior: ReturnBehavior` | no current `ReturnsOnce`/`ReturnsTwice`/`NeverReturns` field: producer gap; never assume ordinary return or infer `noreturn` from a symbol |
| `CallEffects::unwind_behavior: UnwindBehavior` | no current `CannotUnwind`/`MayUnwind` field: producer gap; never assume either behavior |
| `CallEffects::memory_effect: CallMemoryEffect` | no current `None`/`ReadOnly`/`WriteOnly`/`ReadWrite`/`Unknown` field: producer gap; even `Unknown` must be explicit |
| `CallEffects::convergent` | no current field: producer gap; never default `false` |
| `CallEffects::cannot_duplicate` | no current field: producer gap; never default `false` |
| `operand_bundles[].kind: OperandBundleKind` | no current `Deopt`/`Funclet`/`GcTransition`/`Assume` carrier: producer gap |
| `operand_bundles[].first_bundle_operand: uint32_t` | no current descriptor-visible bundle operand range: producer gap |
| `operand_bundles[].operand_types: vector<TypeId>` and descriptor `CallBundleOperand` values | no current typed bundle vector/value operands: producer gap; text may not synthesize them |

Presence means preserve and validate exact agreement; absence of any semantically
required field fails closed. In particular, the importer cannot manufacture
apparently conservative defaults: return/unwind/memory/duplication facts affect
CFG, optimization legality, and observable behavior.

The importer must not compute GP/FP register classes, HFA/eightbyte placement,
shadow space, stack offsets, return registers, hidden-argument locations, or
call moves. If LIR already carries producer-computed ABI observations, preserve
them as non-authoritative origin/audit facts or reject contradictions; do not
publish them as canonical placement.

In particular, `LirCallArg::aarch64_hfa_lane_count`,
`aarch64_hfa_lane_index`, and `aarch64_stack_align_bytes` must never populate
canonical call operands, ABI classes, or placement. They may be copied only to
explicit non-authoritative origin/audit attachments for parity diagnostics.

`va_start`, `va_end`, `va_copy`, and `va_arg` stay explicit. Aggregate `va_arg`
must preserve the requested aggregate type rather than expanding into target
save-area loads in the importer.

## 8. Memory, addresses, atomics, and aggregates

Addresses are ordinary typed values plus explicit semantic provenance seeds:
stack object, global, function, string, label, or unknown pointer. GEP preserves
the source element type and full index path. The importer may validate a
constant projection but does not collapse it to named leaf slots, synthesize
select chains for dynamic arrays, or publish alias/provenance conclusions.

Loads/stores preserve width through type, alignment, volatile, address space,
and atomic semantics. `memcpy`/`memset` remain operations, including dynamic
sizes; importer scalarization would lose overlap/volatility/effect semantics.

The current C4C `LirInst` variant has no structured atomic load/store/RMW,
compare-exchange, or fence alternative even though legacy BIR and the reference
compiler both require them. Full backend coverage therefore requires upstream
typed LIR additions such as `LirAtomicLoadOp`, `LirAtomicStoreOp`,
`LirAtomicRmwOp`, `LirCmpXchgOp`, and `LirFenceOp`. The importer contract for
those forms is to preserve operation, type/width, pointer/value operands,
success and failure ordering, weak/strong state, volatility, and result mode.
Until those variants exist, recognized textual/builtin atomic encodings must
fail as `UnsupportedSemanticFamily`; silently importing them as ordinary calls
or non-atomic loads/stores is forbidden.

Aggregate values remain aggregate values. `extractvalue`, `insertvalue`, byval,
sret, aggregate loads/stores, and initializers retain type identity and field
paths. Leaf-slot decomposition, byte-storage reinterpretation, HFA lanes, and
copy expansion are later aggregate/memory/ABI responsibilities.

## 9. Intrinsics

Intrinsic import uses a registry keyed by structured intrinsic ID. A name may
be retained only as a debug/origin mirror after that ID is selected. Each
descriptor declares result count and
types, operand roles/types, memory read/write effects, volatility, required
feature as a semantic availability constraint, and whether it may trap or has
side effects. Unknown or signature-mismatched intrinsics fail.

Coverage must include memory operations, stacksave/restore, variadic operations,
integer abs, pointer masking, scalar FP operations, integer bit operations,
overflow operations, FP classification, CRC, vector/SIMD operations, barriers,
cache maintenance, and pause/yield hints. Runtime helper selection for i128/f128
is not importer work; source operations retain i128/f128 semantics.

## 10. Inline assembly semantic payload

Raw-stage core `InlineAsm` must preserve, without physical placement:

- template bytes and dialect;
- `side_effects`, volatile, align-stack, and goto state when represented;
- result type and ordered outputs, inputs, read/write and tied relationships;
- structured alternatives, names, early-clobber/commutative modifiers,
  matching operands, immediate/memory/address/register classes, plus optional
  raw constraint spelling as a non-authoritative mirror;
- clobbers including `memory` and condition codes;
- operand types, values, symbol identities, address spaces, and goto `BlockId`s;
- template named references and modifiers;
- validated structured `insn_r` payload without choosing physical registers.

Target-independent constraint validation/normalization may be a dedicated
canonical pass; it consumes structured tokens rather than reparsing source
text. Target-specific constraint satisfaction belongs to preparation; register
assignment, spill/reload, target opcode selection, and encoding belong to
MIR/later target stages. Unsupported constraints are structured import errors; an
`unsupported_facts` string bag is not acceptable final authority.

Today's LIR cannot satisfy that full payload for all source forms.
`LirInlineAsm` has ordered value-ID operands but no operand types/clobbers or
side-effect bit. `LirInlineAsmOp` has clobbers, side effects, and optional
`insn_r`, but its arguments are one rendered `args_str`; neither form carries
parsed alternatives, symbolic operand names, asm-goto blocks, address spaces,
or complete tied/read-write relationships structurally. Compatibility parsing
cannot create those facts; this is a producer gap and import fails.

## 11. Raw-only forms and ownership

| Raw-only form | Why import permits it | Mandatory eliminating/validating owner |
|---|---|---|
| arbitrary integer widths and exact FP literals | source precision must survive | `legalize` |
| old/new duplicate LIR operation shapes | staged LIR migration | importer unifies immediately; no duplicate BIR form escapes |
| typed non-canonical compare/cast/operator | source enum may require canonical normalization | `legalize` then `scalar`; raw spelling is never semantic authority |
| phi forward references | cyclic SSA | importer resolves exact `EdgeKey` identities; `ssa` validates canonical phi placement |
| raw switch and indirect target set | CFG not normalized | `cfg` |
| multi-index GEP and address-space-rich access | semantic address not canonical | `memory` normalizes the GEP using aggregate layout; it is the single first owner |
| aggregate insert/extract and byval/sret markers | decomposition is premature | `aggregate` |
| semantic/feature intrinsic | registry canonicalization pending; name is mirror-only | `intrinsics` |
| structured raw inline-asm constraints | canonical validation is pending but target placement is later | `legalize` validates/normalizes target-independent tokens; `preparation/inline_asm` later places operands |

“Raw-only” never means malformed, untyped, identity-free, or silently lossy.

## 12. Failure and determinism

Import is module-transactional. No successful functions/globals are returned
when any module member fails. Diagnostics should accumulate independent
validation failures where safe, then stop before builder mutation when source
structure is unreliable. After mutation begins, one failure poisons the
transaction; cleanup destroys all unpublished state.

Determinism requirements:

- assign BIR IDs by declared source order, not hash iteration order;
- sort only unordered compatibility maps by stable semantic key before use;
- preserve function block/instruction order and call argument order;
- define duplicate switch/initializer/symbol behavior explicitly;
- order diagnostics by phase, module ordinal, function, block, instruction,
  and field;
- never expose pointer addresses, hash seeds, or unordered iteration in dumps.

Resource exhaustion and builder/publication failures retain their structured
causes. Throwing convenience wrappers, if ever added, must wrap this result API
and must not invent fallback BIR.

Unsupported and exceptional inputs follow the same rule. A known-but-not-yet
modeled semantic family returns `UnsupportedSemanticFamily` at its exact source
site; malformed payload returns the more specific structural/type error; an
unknown variant is never skipped. `noreturn` calls retain their semantic
attribute and the following `unreachable` terminator. There is currently no
typed LIR exception, landing-pad, unwind edge, or cleanup-pad family, so the
importer must reject any compatibility encoding that attempts to smuggle one
through ordinary branches or calls until a first-class source contract exists.

## 13. Backend semantic coverage ledger

This table is the importer review checklist. “Deferred” means the importer
preserves a typed semantic carrier and names its owner; it does not mean the
feature is omitted.

| Semantic family | C4C LIR/import evidence | Reference evidence | Import disposition |
|---|---|---|---|
| scalar constants, integer/FP arithmetic, unary, casts, comparisons | `ir.hpp`; `scalar.cpp`, `types.cpp` | `ir/instruction.rs::{BinOp,UnaryOp,Cast,Cmp}` | current host-width constants cannot carry exact i128/f80/f128: **source gap**; typed operations canonicalize later |
| values, params, copies, select, phi | named params, `LirSelectOp`, `LirPhiOp`; no first-class current `Copy` | `Copy`, `Select`, `Phi`, `ParamRef` | stable IDs and deferred phi fixups; explicit copy/paramref is a **source gap**, never inferred as ABI movement |
| blocks, branch, conditional, return, switch, indirect branch, unreachable | `LirTerminator`; bootstrap `lir_to_bir.cpp` | `Terminator` enum; `backend/generation.rs` | complete typed terminator import; CFG transforms deferred |
| types, structs, arrays, vectors, opaque/packed, i128/f80/f128 | `LirTypeRef`, `LirStructDecl`; much compound identity remains text | `common/types.rs`, `ir/lowering/types*.rs` | predeclare recursive identities; union kind/address spaces/function components and exact extended constants remain **source gaps** |
| globals, strings, linkage, TLS, alignment | `LirGlobal`, `LirStringConst`; no structured TLS/section/visibility suite | `ir/module.rs`, `backend/*/codegen/globals.rs` | predeclare then initialize; TLS/common/weak/section/used metadata are **source gaps** |
| nested/relocation-aware initializers | current `init_text` plus referenced-function ID side vector; partial importer parser | `ir/lowering/global_init*.rs` | **source gap**: add structured initializer/relocation tree; text cannot supply missing topology/relocations |
| stack objects, static/dynamic alloca, stack save/restore | `LirStackObject`, `LirAllocaOp`; memory files | `Alloca`, `DynAlloca`, `StackSave`, `StackRestore` | preserve lifetime/alignment; frame deferred |
| load/store, volatile, address spaces | `LirLoadOp`, `LirStoreOp` lack volatile/alignment/address-space fields | `Load/Store` with `AddressSpace` | **source gap** for access attributes; provenance remains later analysis |
| GEP/global/function/string/label/block addresses | `LirGepOp` has string indices; label address is raw `blockaddress(...)` text | `GetElementPtr`, `GlobalAddr`, `LabelAddr` | **source gap** for typed index and block-address constants; folding deferred |
| memcpy/memset/memmove | typed memcpy/memset LIR ops; no memmove variant | `Memcpy`; intrinsic lowering | preserve volatility/dynamic size where present; **source gap** for memmove and access alignment |
| direct/indirect calls, fn pointers, varargs, effects, bundles | `LirCallOp` has partial signature/args but no complete effects/bundles; `calling.cpp`, `call_abi.cpp` | `Call`, `CallIndirect`, `CallInfo` | **source gap**: require complete structured callee/args/result, every `CallEffects` field, attributes, and typed bundles; placement remains deferred |
| `va_start/end/copy/arg`, aggregate va_arg | typed LIR ops; `calling.cpp` | `VaStart/End/Copy/Arg/ArgStruct` | preserve explicit semantic operations; target va-list plan deferred |
| aggregates, byval/sret, extract/insert, complex returns | extract/insert and byval exist; sret/complex-lane semantics are incomplete | aggregate call metadata and second-return carriers | preserve available aggregate semantics; explicit sret/logical complex multi-result carriers are **source gaps** |
| atomics and fences | legacy `bir.hpp::AtomicOperation`; no typed current LIR variants | `AtomicLoad/Store/Rmw/Cmpxchg/Fence` | **source gap**: add typed LIR; never weaken to ordinary memory/call |
| vectors/SIMD/shuffle | vector LIR ops; intrinsic metadata in legacy BIR | `IntrinsicOp`, architecture intrinsic modules | preserve vector type/lane/mask and intrinsic descriptor |
| inline asm and asm goto | current variants have complementary but incomplete payloads | `Instruction::InlineAsm`, `backend/inline_asm.rs` | **source gap** for structured operands/types/ties/names/goto/address spaces; physical placement is deferred to preparation/MIR |
| intrinsics, barriers, cache, hints | module need flags plus underspecified named `LirIntrinsic`; legacy `IntrinsicOperation` | `ir/intrinsics.rs`, backend intrinsic modules | **source gap** for semantic intrinsic IDs/effects/signatures; unknown/underspecified input fails |
| debug/source coordinates | LIR currently has limited coordinates | reference `BasicBlock::source_spans` | preserve when source adds them; no fabricated locations |
| constructors/destructors, aliases, visibility, sections, symver, top-level asm | not all represented in current `LirModule` | `ir/module.rs` | **source gap**: add structured module metadata; top-level asm additionally needs typed `SymbolId` dependencies and a settled dependency-role schema |
| exception/unwind/EH edges | no current LIR family | no complete portable carrier in reviewed reference IR | explicitly unsupported until language/runtime contract adds typed forms |

Full compiler-backend coverage is not proven while a row is marked source gap.
The importer design closes the boundary by requiring an explicit typed producer
addition or a hard diagnostic, never by reconstructing semantics downstream.

## 14. Legacy source anchors and lessons

Primary C4C anchors inspected:

- `src/codegen/lir/ir.hpp`, `types.hpp`, and `operands.hpp`: actual producer
  schema and complete current variant list.
- `src/backend/legacy/lir_to_bir.hpp` and `lir_to_bir.cpp`: legacy public
  `BirLoweringOptions`, `BirLoweringResult`,
  `try_lower_to_bir_with_options`, `analyze_module`, and `lower_module`.
- `src/backend/legacy/bir.hpp`: legacy types, values, globals, locals, calls,
  memory addresses, intrinsic/inline-asm/atomic payloads, instructions,
  terminators, functions, and module schema; concrete anchors include `Value`,
  `Global`, `MemoryAddress`, `CallInst`, `InlineAsmMetadata`,
  `IntrinsicOperation`, `AtomicOperation`, `Inst`, `Terminator`, `Function`, and
  `Module`.
- `src/backend/legacy/bir_*_view.*`, `bir_route*.cpp`, and `query.*`:
  downstream queries exposing identity, CFG, comparison, call, publication,
  return, select, and memory requirements.
- `src/backend/legacy/prealloc/{legalize,liveness,out_of_ssa,atomics,intrinsics,inline_asm,call_plans,frame_plan,variadic_entry_plans,object_data}.cpp`
  and `prealloc/regalloc/`, `prealloc/stack_layout/`: downstream evidence used
  only to identify semantics the importer must preserve, not work it should do.

Current partial-importer anchors inspected:

- public bootstrap `src/backend/bir/lir_to_bir.hpp` and
  `src/backend/bir/lir_to_bir.cpp`, especially `ImportOptions`, `ImportError`,
  `validate_module_surface`, `validate_function`, `lower_terminator`, and
  `lower_lir_to_raw_bir`;
- this directory's `analysis.cpp`, `context.cpp`, `types.cpp`, `globals.cpp`,
  `global_initializers.cpp`, `module.cpp`, `cfg.cpp`, `scalar.cpp`,
  `aggregate.cpp`, `calling.cpp`, `call_abi.cpp`, and `lowering.hpp`;
- `memory/{addressing,coordinator,intrinsics,local_gep,local_slots,provenance,value_materialization}.cpp`
  plus `memory_helpers.hpp` and `memory_types.hpp`.

The partial importer symbol anchors used for behavior tracing were
`BirFunctionLowerer::lower`, `lower_block_phi_insts`, `lower_block_insts`,
`lower_block_terminator`, `lower_scalar_family_inst`, `lower_call_inst`,
`lower_runtime_intrinsic_inst`, `lower_memory_gep_inst`,
`lower_memory_load_inst`, `lower_memory_store_inst`,
`lower_memory_memcpy_inst`, and `lower_memory_memset_inst`.

The large partial importer demonstrates required edge cases, but it also mixes
import with ABI classification, CFG pattern following, scalar folding,
aggregate decomposition, synthesized selects, provenance publication, and
local-slot materialization. Those behaviors are evidence for later pass
requirements, not the target importer architecture.

## 15. Reference compiler anchors and adopt/reject decisions

Reviewed reference anchors:

- `ref/claudes-c-compiler/src/ir/instruction.rs`: value/operand model, complete
  `Instruction` and `Terminator` variants, `CallInfo`, atomics, and inline asm.
- `ref/claudes-c-compiler/src/ir/module.rs`: module/function/global surface and
  linkage/constructor/alias/section metadata.
- `ref/claudes-c-compiler/src/ir/intrinsics.rs` and `ir/ops.rs`: typed
  intrinsic, arithmetic, comparison, and atomic operation enums.
- `ref/claudes-c-compiler/src/ir/lowering/README.md`, `func_lowering.rs`,
  `global_init*.rs`, `expr_calls.rs`, `expr_atomics.rs`, `stmt_asm.rs`, and
  `stmt_control_flow.rs`: producer-side construction ordering and semantic
  families.
- `ref/claudes-c-compiler/src/backend/generation.rs`, `traits.rs`,
  `liveness.rs`, `regalloc.rs`, `stack_layout/`, and architecture
  `codegen/{memory,calls,variadic,atomics,intrinsics,inline_asm,globals}.rs`:
  consumer evidence for facts that must survive import.

Exact reference control points were `Lowerer::lower` and its documented
prepass/signature/body ordering, `Instruction::dest`/operand visitors,
`generate_module`, `generate_function`, `generate_instruction`, and
`generate_terminator`.

Adopt:

- closed instruction/terminator variants with typed operands and stable IDs;
- shared direct/indirect call semantic metadata;
- explicit atomic orderings/result modes, variadic ops, label addresses,
  address spaces, and inline-asm goto/symbol payloads;
- predeclare-first module construction and relocation-aware initializers;
- per-instruction source coordinates and complete operand visitors as verifier
  infrastructure.

Reject or relocate:

- ABI classifications embedded as backend placement authority in imported IR;
- backend calculation of stack space, GEP folding, register assignment, and
  target emission as importer responsibilities;
- phi elimination before the canonical SSA boundary;
- physical complex-return register carriers as canonical BIR semantics; model
  logical multi-result/aggregate values and defer ABI realization;
- string symbol names as semantic identity when stable source IDs exist.

## 16. Review questions

1. Does new BIR need first-class `TypeId`, `SymbolId`, `GlobalId`, `LocalId`, constant,
   switch, indirect-branch, aggregate, atomic, intrinsic, and inline-asm storage
   before this importer can be implemented without compatibility side tables?
2. Which LIR textual fields remain unavoidable, and what producer milestone
   replaces each with structured identity?
3. Should source verification always be mandatory in production rather than an
   `ImportOptions` bit?
4. How are C symbol namespace collisions between function and object
   declarations represented and diagnosed?
5. Are forward references legal only for phi/CFG cycles, or does LIR permit
   arbitrary value use-before-definition?
6. Is switch case order semantically observable for diagnostics/dumps, and how
   are duplicate equal-value cases rejected upstream?
7. What exact poison/undef/freeze model is required for vectors, aggregates,
   casts, and global initialization?
8. Which alignment, volatile, address-space, no-return, tail-call, and calling
   convention facts are missing from current LIR operations?
9. Will atomics, module directives, debug spans, asm goto, and complex-return
   semantics be added to LIR before importer implementation begins?
10. Which raw intrinsic registry is target-independent, and how are target
    feature requirements represented without selecting an instruction?
11. What exact structured inline-asm constraint schema must the producer supply
    before import, and which target-independent invariants does `legalize`
    validate without reparsing text?
12. Which mirror-parity cases may migration tooling temporarily enable, and
    what producer milestone removes each? No enabled case may survive as
    semantic authority in Raw BIR.
13. Can initializer relocation semantics represent TLS models, function
    addresses, addends, and nested pointer fields without target relocation
    opcode selection?
14. What verifier proves that every `LirInst` variant and every module field is
    either imported or rejected, so future variants cannot be silently skipped?

Acceptance requires resolving these questions together with adjacent core,
verifier, legalize, CFG, SSA, memory, aggregate, intrinsic, and preparation
contracts. Implementation completeness must not be claimed from the current
bootstrap importer or the quarantined legacy importer.

## 17. Current review finding

Already schema-owned in the target design, but not yet implemented by the
bootstrap C++ core: full types/constants, symbols/globals/initializers, closed
opcodes/descriptors, call effects/bundles, structured inline asm, stable IDs,
reservation, and atomic publication. Their implementation absence is planned
work after design freeze, not evidence that this importer should invent another
schema or classify them as producer gaps.

Real current-LIR producer gaps blocking affected backend features:

- structured arbitrary-width constant bits, compound/function/pointer types,
  union/address-space facts, instruction operands, terminator targets, exact
  phi `EdgeKey`s, initializer expressions/relocations, and block
  addresses/label differences;
- atomic/fence forms, complete memory attributes, memmove, and semantic
  intrinsic IDs/descriptors;
- complete calls: callee/argument/result identities and types, calling
  convention, parameter-list kind, tail request, argument/return attributes,
  every `CallEffects` field, and typed operand bundles;
- TLS/common/weak/section/visibility/used, aliases,
  constructors/destructors, symver, and top-level asm with typed `SymbolId`
  dependencies;
- structured inline-asm operands/types/ties/names/symbols/address spaces and
  asm-goto control edges.

Unresolved target-schema choices that must be closed before the corresponding
features are declared design-complete:

- the stable target-independent intrinsic namespace;
- modern asm-goto output-edge value semantics;
- exception/unwind/cleanup-pad control edges beyond call unwind effects;
- the required debug scope/type subset;
- whether top-level-asm `SymbolId` dependencies need definition/use roles;
- whether every object layout is producer-resolved or Raw publication needs a
  separately designed unresolved-layout mechanism.

Non-blocking/settled enough for adjacent review:

- module predeclaration precedes initializer/body import; blocks and result
  identities precede phi/operand fixup;
- all 38 current instruction alternatives and all six terminator alternatives
  have an explicit import-or-fail disposition;
- direct calls use `SymbolId`, semantic locals use `LocalId`, and importer
  tables/capabilities never escape publication;
- instruction/result reservation uses the core `ReservedInst` token, and phi
  fixup matches the exact incoming `EdgeKey` multiset rather than predecessor
  blocks;
- compatibility is production-fail-closed and migration enables only named
  mirror-parity cases with per-case metrics;
- ABI/frame/register allocation/call moves/target selection are forbidden at
  import, with exact later owners identified;
- failure is module-transactional and the only success transition is the single
  `verify_and_publish_raw(ModuleDraft&&)` gate.
