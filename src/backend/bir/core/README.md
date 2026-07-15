# Raw BIR Core Receiving and Ownership Contract

Contract-Status: converging under idea 732; normative NodeKind semantics are external
Implementation-Status: partial
Kind: schema
Phase-ID: A2
Upstream: complete private A1 Raw candidate (current adapter: `ModuleBuilder`)
Downstream: one move-only fully verified target-independent `RawBir`
Owner-Path: `src/backend/bir/core/README.md`
Last-Reconciled-Commit: none

## Purpose

Raw core is the sole typed storage and identity authority for phase-A semantic
facts after import. This contract assigns every current LIR instruction,
terminator, and metadata family exactly one typed core owner or an explicit
validation-only non-destination, while truthfully recording whether that owner
is checked in, partial, or missing.

Architecture ownership is not implementation evidence. A valid current LIR row
that the bootstrap rejects remains unreceived. Current LIR is complete and
immutable for this route; missing receipt belongs to core and/or the importer.

## Durable Node and Pass Model

The durable BIR shape is a flat arena-owned node. `InstId` is the stable
function-arena identity and is not repeated inside `InstData`; slot addresses,
object pointers and vector positions never identify a node. Each node stores a
closed `NodeKind`, one closed payload alternative, ordered input-only
`operands`, and bootstrap result compatibility described below. A payload owns
kind-specific parameters such as a predicate, callee, source authority or
opaque inline-assembly bytes. It does not independently decide the node's
semantic family, arity, effects or phase legality.

Concrete type is a `ValueDef` fact. It remains outside kind-only traits because
the result type of a call, cast, phi or overloaded operation can depend on a
signature, payload, inputs or an explicitly stored value fact. This is the
same semantic distinction used by LLVM-style IR: operations compare stable
kind and typed semantic facts, never C++ object addresses or pointer identity.

### One NodeKind schema authority

The [normative NodeKind/tag contract](../../../../docs/backend/bir_node_kind_tag_algebra_and_phase_vocabulary.md)
alone defines the six axes, tag meanings, stage vocabularies, transition rules,
identity gate, and common verifier obligations. This core document owns only
the concrete checked-in storage and helper surface.

[`ir.hpp`](ir.hpp) defines `NodeKind` as the durable closed vocabulary;
`Opcode` is only a compatibility alias. Its single C++20 registry is authored
with named `NodeKindSpec` fields, typed stage/arity/refinement policies, a
`consteval` entry builder, local relational validation, and a registry-wide
completeness check. Descriptors and all compile-time/runtime queries derive
from that registry. There is no traits-specialization mirror, generated table,
or independently maintained stage mask.

Pass-facing queries include `node_kind_schema<K>()`,
`node_has_tag_v<K, Tag>`, `node_kind_admitted_in_v<K, Stage>`, and
`is_ssa_eligible_in_v<K, Stage>` with runtime equivalents, plus the derived
effect/shape and payload/arity helpers in `ir.hpp`. An unknown runtime kind,
tag, or stage is a known query failure, never permission to continue. Static
SSA eligibility does not prove graph SSA; B4 and its verifier own that dynamic
fact.

A transformation dispatches explicitly over its closed admitted input
vocabulary. Helpers remove duplicate classification tables; they do not permit
silent pass-through, default success, or an implicit remainder. Adding a kind
requires one reviewed registry entry, every affected explicit pass-matrix row,
and nearby positive and negative coverage. Shared storage never implies
admission to a later stage.

### Results and compatibility storage

`InstData::operands` contains input uses only; outputs are never encoded as
operand indices. The checked-in `InstData::results` vector and
`InstResultDef::{instruction,result_index}` remain bootstrap compatibility
storage for ordered generic results. They are not the durable excuse for an
unbounded result-vector architecture, and `ValueDef::type` remains the concrete
type authority.

The preferred durable disposition is one ordinary result value per node.
Operations that are semantically multi-result should normalize into one
aggregate result plus explicit projection nodes, or into another reviewed
single-result form. If measurement later proves that genuine compact
multi-result nodes are required, a compact `ResultSpan` or external
revision-keyed result table is a separately approved follow-up; it must replace,
not silently coexist with, generic result-vector authority.

### Phase vocabulary and external products

The six published-stage meanings and transitions are consumed by reference
from the [normative contract](../../../../docs/backend/bir_node_kind_tag_algebra_and_phase_vocabulary.md).
Core does not redefine their admitted sets. In particular, `Prepared` is the
C-phase immutable admission/reference envelope over the exact Canonical graph
and exact target/preparation products; it is not a second mutable instruction
graph. E4's later MIR-readiness capability is bound to an `Allocated` revision
and must not be called “Prepared BIR.”

Raw-to-Canonical is a target-independent vocabulary transition, not a retag of
the same permissive graph. The historical accepted phase-B contract
([idea 736](../../../../ideas/closed/736_bir_phase_b_canonical_document_convergence.md))
orders B1 legalization, B2 scalar normalization, B3 CFG normalization, B4 SSA,
B5 memory normalization, B6 aggregate normalization and B7 intrinsic/helper
normalization, followed by the distinct B8 verification/publication boundary.
B4 owns SSA construction after the required CFG and dominance facts exist;
SSA is a Canonical property, not a phase-A import prerequisite.

Later ownership remains separate: C selects and verifies target-aware
preparation products; D forms and legalizes pseudos; E owns allocation,
spill/reload and final frame realization; F owns MIR lowering and emission.
Core `NodeKind` schema queries may state stable legality/effect facts, but they do not
absorb target selection, pseudo expansion, register assignment, frame placement
or MIR ownership.

Analyses and phase products are immutable external maps keyed by the exact
module epoch/revision, ordered function revisions, options and applicable
schema/target fingerprints. They do not become fields on core nodes. Mutation
invalidates or explicitly preserves those products for the new exact key;
stale, foreign or merely compatible-looking products are rejected rather than
retargeted.

This section publishes the durable contract and the schema/helper foundation
that is checked in now. It does not claim that the full A inventory, B1-B8
pipeline, C-F products, result normalization or all future node kinds are
implemented.

## Owns

- module/function-local stable typed IDs and generation-checked storage;
- deterministic semantic iteration order independent of slot reuse or hashes;
- one typed owner for types, constants, symbols, globals, functions, locals,
  blocks, instructions, values, initializers, data objects and origin records;
- closed instruction payloads, ordered input operands and bootstrap-compatible
  result bindings, exact def-use,
  one terminator per block and terminator-only CFG successor authority;
- private draft storage and the immutable storage behind published `RawBir`;
- target-independent source semantics needed by later canonical passes.

Old/new LIR twins converge on one semantic opcode/payload owner. They never
create duplicate BIR families or compatibility side graphs.

## Does Not Own

- LIR, import dispatch/maps/diagnostics, or memory-sub-boundary validation;
- the full A2 verification rules or authority to publish by bypassing the
  shared verifier;
- `LirModule::target_profile`, rendered `data_layout`, producer caches/indexes,
  intrinsic requirement flags, or layout observations as semantic Raw state;
- ABI classification, constraint interpretation, selected target opcodes,
  register classes/homes, spill/reload, frame offsets/actions, MIR, emission,
  object/link state, or parsed assembly;
- authoritative predecessor/dominator/loop/liveness/provenance/memory-effect/
  call-graph caches;
- identity inferred from names, addresses, pointer values, vector positions,
  rendered text, hash order, or dense analysis indices.

## Inputs

Core accepts one complete move-only private Raw candidate from the sole
[A1 importer](../lir_to_bir/README.md). The candidate contains typed specs,
reservations, deterministic orders and validation-only dispositions for the
exact 38/6/18 inventory rooted at [`ir.hpp`](../../../codegen/lir/ir.hpp).

All required definitions, uses, blocks, symbols, types, declarations,
initializers and optional forms must be resolved before publication. Importer
maps, rendered compatibility identity, unresolved fixups, partial functions,
diagnostic-only proofs, target context and allocation facts are not inputs.

## Outputs

The sole public phase-A output is one move-only immutable `RawBir` minted by
the shared full A2 gate from the exact consumed draft revision. It exposes
typed read-only views and stable deterministic iteration. It contains no active
editor, reservation, unresolved reference, importer side map, target/profile/
layout semantics, preparation product or allocation state.

### Output handoff matrix

| Output | Exact consumer | Required binding and acceptance | Failure / forbidden escape |
|---|---|---|---|
| private Raw candidate | shared [A2 verifier](../verify/README.md) | one exact module epoch/revision, closed typed owners, complete orders/def-use/CFG, no active capabilities | gate failure consumes/discards the candidate and publishes no module, subset or proof token |
| move-only verified `RawBir` | [B1 legalize](../passes/legalize/README.md) | immutable published view; every current LIR fact has one owner and all admitted Raw-only forms are typed | B1 receives no draft, importer maps, hidden side table, malformed/unsupported valid row or target/allocation fact |
| read-only entity/views | later Raw analyses and canonical passes | IDs carry module/function ownership and exact revision; iteration follows explicit order | stale revision/foreign owner rejects; views cannot mutate or create semantic caches |
| origin/debug attachments | diagnostics/audit consumers only | non-authoritative references to already-owned semantic entities | names/text/sites cannot resolve or replace semantic identity |

## Adjacent-Stage Contract

The [top-level importer](../lir_to_bir/README.md) owns closed dispatch and the
private transaction; the [memory sub-boundary](../lir_to_bir/memory/README.md)
only contributes row validation/specs. Core owns storage, never their dispatch.
The shared [A2 verifier](../verify/README.md) alone owns
the one Raw publication gate and Raw acceptance. No builder method,
core constructor, diagnostic verifier or importer may bypass that gate.

B1's exact consumer clause accepts only an immutable published `RawBir`. The
historical phase-A ownership contract is recorded in
[closed idea 735](../../../../ideas/closed/735_bir_phase_a_import_raw_document_convergence.md).
It is historical architecture evidence rather than current implementation
completeness.

## Exhaustive Instruction Receiving Matrix

This matrix preserves the phase-A target inventory accepted under closed idea
735. Its `Checked-in disposition` column is historical and is not a current
code inventory; the current implementation boundary is stated explicitly in
`Implementation State` below. In particular, it must not override the landed
16-kind NodeKind schema described above.

Each row has one target core owner. `missing` means the typed owner is designed
but not checked in; `partial` names the exact checked-in subset. No row is
complete merely because import rejects it.

| Alternative | Single typed core owner | Checked-in disposition | Required owner invariant |
|---|---|---|---|
| `LirInst::LirConstInt` | one `ConstantId` in `ConstantStore` plus ordinary result binding | missing container | exact current type/value bits, interning identity and one result definition |
| `LirInst::LirConstFloat` | one floating `ConstantId` in `ConstantStore` plus result binding | missing container | exact current value bits/type and one result definition |
| `LirInst::LirLoad` | one shared `InstData(Opcode::Load, LoadPayload)` | missing container | typed pointer use, result definition and semantic load fields |
| `LirInst::LirStore` | one shared `InstData(Opcode::Store, StorePayload)` | missing container | ordered typed pointer/value uses and stored type |
| `LirInst::LirBinary` | one shared typed binary/unary `InstData` owner | missing container | closed opcode, arity, operand/result types and flags actually present |
| `LirInst::LirCast` | one shared `InstData(Opcode::Cast, CastPayload)` | missing container | exact cast kind derivation, from/to type and use/result |
| `LirInst::LirCmp` | one shared `InstData(Opcode::Compare, ComparePayload)` | missing container | typed predicate domain, operands and boolean result |
| `LirInst::LirCall` | one shared `InstData(Opcode::Call, CallPayload)` | missing container | symbol-or-value callee, function type, ordered arguments and zero/one result |
| `LirInst::LirGep` | one shared `InstData(Opcode::GetElementPtr, GepPayload)` | missing container | base type/pointer and complete ordered index path without folding |
| `LirInst::LirSelect` | one shared `InstData(Opcode::Select, SelectPayload)` | missing container | boolean condition and equal typed arm/result edges |
| `LirInst::LirIntrinsic` | one `InstData(Opcode::Intrinsic, IntrinsicPayload)` | missing container | target-independent registry identity, signature, effects and ordinary edges |
| `LirInst::LirInlineAsm` | one shared `InstData(Opcode::InlineAsm, InlineAsmNode)` | partial container; generic opcode/payload exists, legacy wiring absent | ordinary edges and opaque exact asm/constraint payload; no parsed meaning |
| `LirInst::LirMemcpyOp` | one `InstData(Opcode::Memcpy, MemcpyPayload)` | missing container | ordered dst/src/size uses and volatility |
| `LirInst::LirVaStartOp` | one `InstData(Opcode::VaStart, VaStartPayload)` | missing container | typed va-list pointer use and semantic effect |
| `LirInst::LirVaEndOp` | one `InstData(Opcode::VaEnd, VaEndPayload)` | missing container | typed va-list pointer use and semantic effect |
| `LirInst::LirVaCopyOp` | one `InstData(Opcode::VaCopy, VaCopyPayload)` | missing container | ordered typed destination/source uses and effect |
| `LirInst::LirStackSaveOp` | one `InstData(Opcode::StackSave, StackSavePayload)` | missing container | one pointer-like result and exact order, no frame home |
| `LirInst::LirStackRestoreOp` | one `InstData(Opcode::StackRestore, StackRestorePayload)` | missing container | one typed saved-pointer use and exact order |
| `LirInst::LirAbsOp` | one `InstData(Opcode::Abs, AbsPayload)` | missing container | exact current integer type, input/result and no invented flags |
| `LirInst::LirIndirectBrOp` | one `IndirectJumpTerm` owner, never an ordinary instruction owner | missing container | reconciled last-op/sentinel form, target operand and ordered successor set |
| `LirInst::LirExtractValueOp` | one `InstData(Opcode::ExtractValue, ExtractValuePayload)` | missing container | aggregate/result type and exact field index |
| `LirInst::LirInsertValueOp` | one `InstData(Opcode::InsertValue, InsertValuePayload)` | missing container | aggregate/element types, exact index and ordinary edges |
| `LirInst::LirLoadOp` | same one shared `Load` owner as `LirLoad` | missing container | structured current kind/type and ordinary edges; no duplicate opcode |
| `LirInst::LirStoreOp` | same one shared `Store` owner as `LirStore` | missing container | structured current kind/type and ordinary edges; no duplicate opcode |
| `LirInst::LirMemsetOp` | one `InstData(Opcode::Memset, MemsetPayload)` | missing container | dst/byte/size uses and volatility, no scalarization |
| `LirInst::LirCastOp` | same one shared `Cast` owner as `LirCast` | missing container | closed `LirCastKind`, exact types/use/result |
| `LirInst::LirGepOp` | same one shared `GetElementPtr` owner as `LirGep` | missing container | element type, pointer, `inbounds` and admitted ordered current path |
| `LirInst::LirCallOp` | same one shared `Call` owner as `LirCall` | missing container | structured callee/link ID/signature/args/extensions; mirrors non-authoritative |
| `LirInst::LirBinOp` | same one shared binary/unary owner as `LirBinary` | missing container | typed opcode ref, exact type/arity and ordinary edges |
| `LirInst::LirCmpOp` | same one shared `Compare` owner as `LirCmp` | missing container | float domain, typed predicate/type and ordinary edges |
| `LirInst::LirPhiOp` | one `InstData(Opcode::Phi, PhiPayload)` | missing container | typed result and one operand per exact terminator-derived `EdgeKey` |
| `LirInst::LirSelectOp` | same one shared `Select` owner as `LirSelect` | missing container | typed condition/arms/result and no fused comparison authority |
| `LirInst::LirInsertElementOp` | one `InstData(Opcode::InsertElement, InsertElementPayload)` | missing container | vector/element/index types and ordinary edges |
| `LirInst::LirExtractElementOp` | one `InstData(Opcode::ExtractElement, ExtractElementPayload)` | missing container | vector/index/result types and ordinary edges |
| `LirInst::LirShuffleVectorOp` | one `InstData(Opcode::ShuffleVector, ShuffleVectorPayload)` | missing container | two vector inputs, exact mask and result shape |
| `LirInst::LirVaArgOp` | one `InstData(Opcode::VaArg, VaArgPayload)` | missing container | va-list pointer, result type/value and no ABI expansion |
| `LirInst::LirAllocaOp` | one `InstData(Opcode::Alloca, AllocaPayload)` plus `LocalId` result relation | missing container | element/count/alignment, static/dynamic form and no frame placement |
| `LirInst::LirInlineAsmOp` | same one checked-in `InlineAsm` owner plus typed role/index attachment | partial container; ordinary edges/payload exist, role/index attachment missing | generic values, exact types, roles/indices, opaque original text/clobbers/effects; `insn_r` audit-only |

## Exhaustive Terminator Receiving Matrix

| Alternative | Single typed core owner | Checked-in disposition | Required owner invariant |
|---|---|---|---|
| `LirTerminator::LirBr` | one checked-in `JumpTerm` | checked-in container | one valid owned target and sole successor slot |
| `LirTerminator::LirCondBr` | one checked-in `CondJumpTerm` | checked-in container; importer wiring missing | one boolean value and ordered true/false target slots, including equal targets |
| `LirTerminator::LirRet` | one checked-in `ReturnTerm` | checked-in container; importer only wires void | zero/one value exactly matching semantic function result |
| `LirTerminator::LirSwitch` | one `SwitchTerm` | missing container | selector, default, ordered typed cases and distinct successor slots |
| `LirTerminator::LirIndirectBr` | one `IndirectJumpTerm` | missing container | target operand and complete ordered conservative target set |
| `LirTerminator::LirUnreachable` | one checked-in `UnreachableTerm` | checked-in container | no successor or fallthrough |

## Exhaustive Metadata-Family Receiving Matrix

| Family key | Single typed core owner or non-destination | Checked-in disposition | Required owner invariant |
|---|---|---|---|
| `module-context` | validation/origin/parity-only; no semantic core owner | validation-only non-destination | `target_profile`/rendered `data_layout` cannot influence Raw; C1/C2 own later target selection/layout |
| `stable-identities` | one ID family each for type/constant/initializer/symbol/global/function/local/block/inst/value/origin | partial: function/block/inst/value IDs exist; remaining ID families missing | epoch/owner/slot/generation identity, foreign/stale rejection and no namespace aliasing |
| `operand-kinds` | one closed typed `Operand`/constant/symbol/block reference family | missing container beyond checked-in `ValueId` edges | each current kind maps to a typed owner or explicit error; raw text creates no identity |
| `type-system` | one interned `TypeStore` and named aggregate owner | partial: Void, I1/I8/I16/I32/I64, F32/F64, Pointer exist | structural interning, exact semantic kind/width/shape and no target layout semantics |
| `functions-signatures` | one `FunctionData`/`FunctionSignature` plus symbol and ordered parameter values | partial: basic function/signature/declaration/parameter storage exists | complete current flags, link identity, params/results and declaration/body coherence |
| `blocks-cfg-order` | one `BlockData` order plus exactly one terminator and explicit entry identity | partial: block/inst order and terminator exist; explicit entry/full CFG owners incomplete | deterministic order, terminator-only successors and no vector-position entry identity |
| `values-def-use` | one `ValueData` definition family and exact `DefUseStore` | partial: parameter/inst-result definitions and generic asm edges exist; complete def-use missing | one definition, typed uses, forward resolution and exact revision ownership |
| `stack-objects-allocas` | one `LocalStore`/`LocalId` plus typed `Alloca` instructions | missing container | object identity/type/alignment/VLA and inline/hoisted order, no frame placement |
| `globals-objects` | one `SymbolTable` plus `GlobalStore`/`GlobalId` | missing container | current type/link/internal/const/alignment/extern facts and deterministic order |
| `initializers` | one `InitializerStore`/`InitializerId` tied to one global | missing container | exact current ID/payload disposition; no text-parsed topology or relocation identity |
| `strings` | one typed data/string object plus ordinary symbol/global identity | missing container | exact current bytes/spelling classification, length and source order |
| `externs` | one `SymbolData`/function declaration owner | missing container beyond basic unnamed function shell | stable link identity, type/extensions and coherent declaration merge; maps validation-only |
| `specializations` | one typed specialization/origin record linked to ordinary symbol | missing container | ordered key/origin/mangled identity without a parallel symbol namespace |
| `intrinsic-requirements` | validation-only requirement parity; no semantic core owner | validation-only non-destination | flags agree with actual typed operations/declarations and never synthesize nodes |
| `inline-asm-metadata` | one `InlineAsmNode` plus ordinary edges and typed role/index attachment | partial: opaque payload/edges exist; role/index attachment missing | exact bytes/order/clobbers/effects/roles/indices; no parsed constraints or special values |
| `producer-indexes-caches` | validation/import report only; no semantic core owner | validation-only non-destination | compare deterministically with ordered authority; no pointer/hash/cache identity publication |
| `source-order-origin` | one explicit `IdOrder` per owned entity sequence plus non-authoritative origin attachments | partial: function/block/inst order and debug block name exist | preserve all module/nested source orders and stable diagnostic sites |
| `module-publication` | one private Raw candidate consumed by the shared A2 gate into one `RawBir` | partial: `ModuleBuilder::publish() &&` foundation adapter exists; Step 8 reconciles the final landed candidate/API | exact revision freeze, no active editor/reservation/fixup and atomic whole-module publication |

## Ordered Behavior

1. A1 creates one private module epoch and reserves typed module entities in
   source order: named types, constants/initializers, symbols/globals, functions
   and data objects.
2. Each function reserves locals, parameters, blocks, instructions and result
   values in deterministic order. Reservation fixes typed identity and order;
   definition must match result arity/types exactly.
3. Non-phi payloads are defined with ordinary operand edges. Terminators then
   define the sole successor-slot sequence; phi entries resolve against exact
   edge identities, including parallel edges.
4. All declarations/definitions, initializers, extern merges, optionals,
   forward uses and validation-only dispositions resolve before finish.
5. `finish() &&` freezes one exact private draft revision. It cannot publish.
6. The shared A2 gate consumes that draft exactly once and either mints one
   immutable `RawBir` or publishes nothing.

## Invariants

### Stable identity and ownership

The target ID family is disjoint and owner-scoped: `TypeId`, `ConstantId`,
`InitializerId`, `SymbolId`, `GlobalId`, `FunctionId`, `LocalId`, `BlockId`,
`InstId`, `ValueId` and origin/debug IDs. Module-owned IDs carry a module epoch;
function-local IDs carry `FunctionId`; slot and generation reject stale reuse.
A `ValueId` is parameter- or instruction-result-defined and never aliases a
symbol, constant, local or block.

The checked-in bootstrap already uses owner/slot/generation IDs for functions,
blocks, instructions and values plus generation-checked `SlotMap` and explicit
`IdOrder`. Missing target ID/store families remain design, not implementation.

### Deterministic order and storage

Slot allocation/free-list order is not semantic order. Each owner maintains an
explicit `IdOrder`: module entity/source order, function parameter/local/block
order, block instruction order, instruction operand/result order, terminator
successor-slot order, call argument order, initializer element order and every
other nested sequence. Hash tables are lookup aids only.

### Exact def-use and CFG

Each ordinary value has one typed definition and exact role/indexed use sites.
Edits update payload, operand edges and def-use atomically. Terminators alone
own CFG successors; predecessor, dominance, liveness and provenance are derived
revision-bound analyses. Phi entries bind exact successor/edge identity rather
than predecessor block cardinality.

### Target-independent Raw

Raw retains semantic operations and opaque inline-asm bytes/requirements but no
target/profile/layout-text semantics, ABI placement, constraints product,
selected opcode/register, home, spill/reload, frame, MIR or emission state.
Validation-only rows create no semantic slot or duplicate authority.

## Verification and Publication

Core provides private storage and views, but the shared verifier owns Raw
acceptance. The private Raw candidate is move-only and unpublished. Only the
one A2 publication gate may freeze/check the exact revision and mint `RawBir`;
a public constructor, diagnostic report, `verify(ModuleView)`, cast, or
stage-label change cannot bypass this boundary. The current adapter is
`ModuleBuilder::publish() &&`; Step 8 owns final API reconciliation.

The full gate checks every typed owner, declaration/definition, order, ID,
def-use edge, payload descriptor, terminator/successor, initializer, unresolved
reservation/fixup, validation-only exclusion, and absence of target/allocation
state. Failure consumes/discards the candidate and returns the unchanged shared
publication cause with no partial module or capability.

## Failure and Diagnostics

Core errors distinguish invalid/foreign/stale IDs, missing/duplicate entities,
type/arity/payload mismatch, invalid order membership, unresolved reservation,
declaration/definition conflict, invalid initializer/reference, active editor,
resource exhaustion and publication rejection. They carry stable entity/source
context without using display names as identity.

Builder/editor operations are transactional. A failed append/definition/edit
rolls back every created result/use/order entry. Module failure destroys the
whole unpublished draft. No tombstoned ID, partial function/global, successful
prefix, side graph, hidden fixup, diagnostic proof or stage token escapes.

## Analysis and Invalidation

Core stores revisions and mutation summaries, not derived analysis authority.
Views are bound to an exact immutable revision; editors are private,
capability-scoped transactions. Mutation invalidates analysis products unless
their owner proves preservation for that exact revision change. Raw publication
starts with no importer cache or mutable analysis state.

## Target and ABI Rules

`target_profile` and rendered `data_layout` have no semantic core destination.
Core may store only genuinely target-independent typed facts assigned by a
matrix row; it cannot parse layout/rendering text. C1 independently selects an
exact target profile and C2 derives target layout after Canonical publication.

Core contains no ABI argument/result location, calling-placement plan, target
address selection, constraint binding, register class/home, stack offset,
frame action, target opcode, relocation encoding, MIR or emission record.

## Implementation State

Checked-in headers [`ir.hpp`](ir.hpp), [`type.hpp`](type.hpp),
[`storage.hpp`](storage.hpp), [`builder.hpp`](builder.hpp) and
[`view.hpp`](view.hpp) implement a bounded foundation:

- function/block/instruction/value IDs, generation-checked slots and explicit
  function/block/instruction order;
- parameter/instruction-result definitions and generic instruction
  input-operand/bootstrap-result vectors;
- the closed 16-kind `NodeKind` vocabulary, its payload variant, one C++20
  registry authority, compile-time/runtime query surfaces and fail-closed
  payload/arity verifier preconditions;
- current structured `Type`, constant, symbol/global, function, block,
  instruction, value and selected authority receipts admitted by the bounded
  builders and verifier;
- `JumpTerm`, `CondJumpTerm`, `IndirectJumpTerm`, `SwitchTerm`, `ReturnTerm` and
  `UnreachableTerm` storage;
- basic functions/signatures/declarations, builders/views, revision/stage
  stamps, `RawBir`/`CanonicalBir` wrappers and foundation publication paths.

It does not implement the complete phase-A target inventory, full def-use
store, durable single-result normalization, all B1-B8 transformations, or the
C-F target/pseudo/allocation/MIR chain. Therefore
`Implementation-Status: partial` remains exact: the landed NodeKind contract is
real shared-code infrastructure, but documentation and schema coverage are not
evidence of end-to-end compiler completeness.

## Proof Requirements

- mechanically compare the 38 instruction and six terminator receiving rows
  with current `ir.hpp` in exact order and compare all 18 metadata keys with
  Child A;
- require exactly one nonempty target owner/non-destination and one truthful
  checked-in disposition for every row; old/new twins must name the same owner;
- require exact metadata spine, core-first and applicable detail-heading order;
- resolve all relative links and inspect current headers/importer/verifier
  without changing them;
- require validation-only module context, requirement flags and producer caches
  to publish no semantic owner;
- require stable ID/order/def-use/terminator-only CFG, no target/allocation state,
  private freeze, sole full A2 gate and no builder bypass/partial publication;
- reject checked-in coverage claims for any target owner absent from headers.

## Open Questions

No open question can create a second verifier or authorize an adjacent edit. If
the importer, memory boundary or shared A2 verifier cannot accept these exact
owners, record the named coordinated-boundary seam in `todo.md` and stop until
that owner is explicitly authorized.

## Review Checklist

- [x] Metadata spine, core-first sections and applicable detail order are exact.
- [x] All 38/6/18 rows have one typed owner/non-destination and truthful disposition.
- [x] Old/new twins converge on one semantic opcode/owner.
- [x] Stable IDs, deterministic orders, exact def-use and terminator-only CFG authority are explicit.
- [x] Declarations/definitions, globals, initializers, strings, externs, specializations and storage optionals are owned.
- [x] Validation-only context/caches/flags publish no semantic duplicate.
- [x] Raw is target-independent and unallocated with no text-derived identity.
- [x] Private draft freeze and sole full A2 gate have no builder bypass or partial publication.
- [x] Checked-in partial schema/storage/verifier truth is distinct from missing target owners.
- [x] B1 receives only one move-only fully verified `RawBir`.
