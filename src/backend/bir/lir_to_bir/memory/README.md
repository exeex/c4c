# LIR-to-Raw-BIR Memory Import Sub-Boundary

Contract-Status: under-review
Implementation-Status: absent
Kind: boundary
Phase-ID: A1
Upstream: memory-assigned rows in the sole top-level A1 importer transaction
Downstream: typed memory/object specifications in that same private `ModuleDraft`
Owner-Path: `src/backend/bir/lir_to_bir/memory/README.md`
Last-Reconciled-Commit: none

## Purpose

This document refines validation and construction requirements for the current
memory/object rows owned by the sole top-level
[A1 importer contract](../README.md). It is a subordinate documentation
boundary, not a second importer. It contributes typed specifications to the
parent's one private transaction; it cannot dispatch the whole `LirInst`
variant, own Raw storage, verify or publish a module, or return a partial result.

Current LIR is complete and immutable for this route. This boundary classifies
and transports only facts present in the current rows below. A desired atomic,
address-space, lifetime, relocation, or layout form absent from those rows is
not permission to edit LIR and is not silently invented here.

## Owns

- field-level validation and typed-spec construction for the 11 individually
  named current instruction rows in the input matrix below;
- validation and construction contributions for stack objects, globals,
  initializers, strings, stable identities, types, def-use, nested order, and
  applicable intrinsic-requirement parity;
- exact current alignment, count, volatility, ordering, object, pointer, type,
  and compatibility-payload dispositions where the named LIR fields provide
  them;
- memory-row diagnostics returned into the parent A1 transaction.

Every row retains the exact source/nested order selected by the parent. Each
source identity maps once to the ordinary typed BIR identity and value graph;
this document creates no parallel memory identity namespace.

## Does Not Own

- the parent importer's closed 38/6 dispatcher, module inventory, source-to-BIR
  map lifetime, transaction, or final `ModuleDraft` construction;
- core storage, builders, the full A2 verifier, `RawBir` publication, or any
  independent memory graph, verifier, stage token, or publication path;
- aggregate projection/update rows, variadic rows, calls, general intrinsics,
  inline asm, atomics absent from current `LirInst`, or CFG/terminator rows;
- scalarization, address/GEP folding, provenance inference, alias analysis,
  target/profile selection, target layout, address selection, ABI placement,
  frame layout, register allocation, machine lowering, MIR, emission, object,
  link, or assembler work;
- semantic identity reconstructed from names, renderer text, initializer text,
  pointer values, vector position, caches, hash order, or legacy route records.

`LirExtractValueOp`, `LirInsertValueOp`, and vector element/shuffle rows remain
with the parent matrix's aggregate/vector owners. `LirVa*`, call, intrinsic,
inline-asm, indirect-branch, and terminator rows likewise remain outside this
sub-boundary. That explicit exclusion prevents this file from becoming a
second generic dispatcher.

## Inputs

The only input is a parent-selected row plus the exact structured/current
fields named below. Stable IDs and typed operands are semantic authority.
`TypeSpec` and `LirTypeRef` contribute their current structured facts.
Rendering strings are compatibility payload/mirrors only under the parent
row's explicit rule; they cannot create identity, an address path, initializer
topology, relocation, type shape, alignment, volatility, or effect.

Optional/error forms are explicit: legacy/new load-store-GEP twins, volatile
or nonvolatile copy/set, simple or counted alloca, zero or specified alignment,
VLA/static stack object, declaration/definition global, absent/present
initializer compatibility payload, empty/nonempty string data, and matching or
inconsistent module requirement flags. Missing required current facts poison
the parent transaction.

### Input handoff matrix

Each `Parent row` is a unique reference to the same-named row in the committed
top-level A1 matrix at `10d70b872`. “Spec contribution” means an input to the
one parent-owned draft builder; this document does not own that API.

| Current row | Exact current input and authority | Parent row / single typed Raw owner | Subordinate importer rule and disposition | Stable failure and neighboring proof obligation |
|---|---|---|---|---|
| `LirInst::LirLoad` | S: result/type/pointer IDs | Parent row `LirInst::LirLoad`; one `Load` payload with ordinary use/result edges | validate IDs/type, contribute shared load spec; container+wiring missing | reject missing/foreign pointer, type mismatch or duplicate result; prove valid load beside `LirLoadOp` |
| `LirInst::LirStore` | S: pointer/value IDs and type | Parent row `LirInst::LirStore`; one `Store` payload with ordered uses | validate both uses/type, contribute shared store spec; container+wiring missing | reject unresolved/foreign use or stored-type mismatch; prove beside `LirStoreOp` |
| `LirInst::LirGep` | S: result/base type/pointer/ordered index IDs | Parent row `LirInst::LirGep`; one `GetElementPtr` payload | preserve exact ordered current path without folding; container+wiring missing | reject bad base/index/type/result; prove multi-index path beside `LirGepOp` |
| `LirInst::LirMemcpyOp` | S: typed dst/src/size and `is_volatile` | Parent row `LirInst::LirMemcpyOp`; one semantic `Memcpy` payload | preserve operand order, dynamic size and volatility; container+wiring missing | reject kind/type/size error; prove volatile/nonvolatile beside memset |
| `LirInst::LirStackSaveOp` | S: typed ordinary result | Parent row `LirInst::LirStackSaveOp`; one `StackSave` payload/result | validate one pointer-like definition/order; container+wiring missing | reject duplicate/mistyped result; prove matching restore neighbor |
| `LirInst::LirStackRestoreOp` | S: typed saved-pointer operand | Parent row `LirInst::LirStackRestoreOp`; one `StackRestore` payload | validate owned use/order; container+wiring missing | reject foreign/missing/mistyped use; prove matching save neighbor |
| `LirInst::LirLoadOp` | S: result/pointer operands and `LirTypeRef` | Parent row `LirInst::LirLoadOp`; same single `Load` owner as legacy twin | structured kinds/type drive spec; container+wiring missing | reject semantic `RawText`, missing use or bad result; prove twin convergence |
| `LirInst::LirStoreOp` | S: value/pointer operands and `LirTypeRef` | Parent row `LirInst::LirStoreOp`; same single `Store` owner as legacy twin | structured kinds/type drive spec; container+wiring missing | reject raw/unresolved/mismatched use; prove twin convergence |
| `LirInst::LirMemsetOp` | S: typed dst/byte/size and `is_volatile` | Parent row `LirInst::LirMemsetOp`; one semantic `Memset` payload | preserve byte/size/order/volatility, no scalarization; container+wiring missing | reject bad width/kind/size; prove volatile/nonvolatile beside memcpy |
| `LirInst::LirGepOp` | S: result/pointer/element type/`inbounds`; C: ordered current index strings | Parent row `LirInst::LirGepOp`; same single `GetElementPtr` owner as legacy twin | retain only explicitly admitted current path facts, never infer/fold; container+wiring missing | reject inadmissible path or type/use mismatch; prove twin and `inbounds` neighbors |
| `LirInst::LirAllocaOp` | S: result/element type/optional count/alignment | Parent row `LirInst::LirAllocaOp`; one semantic local/`Alloca` payload | preserve static/dynamic form and exact order, no frame placement; container+wiring missing | reject bad type/count/alignment/result; prove simple/counted/hoisted neighbors |
| `metadata::stack-objects-allocas` | S: `LirStackObject` ID/name/type/alignment/VLA, ordered objects and `LirFunction::alloca_insts` | Parent family `stack-objects-allocas`; one typed local-object owner plus the same `Alloca` instruction owner | predeclare objects and route hoisted allocas through the parent row in source order; container+wiring missing | reject duplicate ID, bad type/order or dropped object; prove static/VLA and inline/hoisted neighbors |
| `metadata::stable-identities` | S: value/block/stack/global/link/struct IDs and invalid sentinels | Parent family `stable-identities`; ordinary typed BIR IDs only | use parent maps, never create a memory ID table; partial shell only | reject duplicate/foreign/invalid identity; prove forward references and object/value neighbors |
| `metadata::operand-kinds` | S: current `LirOperand` kind/text | Parent family `operand-kinds`; typed value/constant/symbol/block owner or explicit error | accept only row-admitted kinds; no text-derived identity; wiring missing | reject `RawText` as semantic address/use; prove SSA/global/immediate/special neighbors |
| `metadata::type-system` | S: current `TypeSpec`, `LirTypeRef`, struct declarations; C: mirrors | Parent family `type-system`; one interned typed owner | resolve existing structured type facts before memory specs; complete container+wiring missing | reject unresolved/conflicting type or mirror authority; prove scalar/pointer/aggregate neighbors |
| `metadata::values-def-use` | S: all row operands/results and parameter/forward definitions | Parent family `values-def-use`; ordinary definitions and exact use edges | reserve/resolve through parent transaction only; bounded asm coverage does not cover memory | reject missing/foreign/duplicate use/def; prove forward and cross-block neighbors |
| `metadata::globals-objects` | S: global/link IDs, type/internal/const/alignment/extern; C: linkage/qualifier/type mirrors | Parent family `globals-objects`; one typed symbol/global/object/declaration owner | predeclare by stable identity/order; mirrors parity-only; container+wiring missing | reject conflict, dangling type or invented metadata; prove decl/def/internal/const neighbors |
| `metadata::initializers` | S: ordered initializer function link IDs; C: current `init_text` payload | Parent family `initializers`; one typed initializer owner with explicit compatibility-payload disposition | resolve existing IDs, never parse text into topology/relocations; container+wiring missing | reject dangling ID or unreceivable payload without partial global; prove zero/symbol/global neighbors |
| `metadata::strings` | S current payload: ordered pool name/raw bytes/byte length; V: cache/counter | Parent family `strings`; one typed string/data object and symbol owner | preserve exact current bytes/spelling classification, length and vector order; container+wiring missing | reject length/name/cache mismatch or hash-order identity; prove empty/embedded/multiple neighbors |
| `metadata::intrinsic-requirements` | V: `need_memcpy`, `need_memset`, `need_stacksave`, `need_stackrestore` | Parent family `intrinsic-requirements`; validation-only parity non-destination | cross-check named rows/declarations, never synthesize operations; wiring missing | reject missing/spurious flag; prove each flag beside its operation and unrelated flags |
| `metadata::source-order-origin` | S: module/object/function/block/instruction and nested operand/index orders; C/V: labels/sites | Parent family `source-order-origin`; typed deterministic orders plus non-authoritative origin | preserve every selected order and stable site; partial shell only | reject hash iteration, missing site or reordered nested path; prove reordered declarations/indices |
| `metadata::module-publication` | V/S: complete parent-validated module and all rows above | Parent family `module-publication`; same one private `ModuleDraft`, then sole full A2 gate | contribute or fail inside parent transaction; no independent finish/publish | inject one fault per row/family and prove whole-module rollback; neighbor is non-memory family in same transaction |

## Outputs

This boundary produces no standalone artifact. A successful row contributes
one typed spec or one validation-only result to the parent-owned transaction.
The parent alone resolves all families and finishes the one private draft.

### Output handoff matrix

| Contribution | Exact consumer and binding | Acceptance | Failure / forbidden escape |
|---|---|---|---|
| typed load/store/GEP/copy/set/stack/alloca specs | parent A1 dispatcher/build transaction, same module epoch and function revision | one core-owned typed node with ordinary edges in parent source order | builder/identity/type failure poisons the parent; no opaque placeholder or prefix draft |
| local/global/string/initializer specs | parent module predeclaration/definition phases, same module epoch and stable symbol/type IDs | one ordinary core owner per parent metadata row; compatibility payload never becomes identity | conflict/unreceivable payload rejects the module; no side object graph |
| parity/cache/requirement checks | parent validation report only, keyed to the same source site | exact agreement with ordered semantic authority | mismatch is a stable import error; no semantic cache/index publication |
| completed memory contributions | same parent-owned private `ModuleDraft` | all memory reservations/fixups resolved before parent finish | this boundary cannot finish, freeze, return, verify, or publish the draft |
| A2-published memory state | shared full A2 gate, exact draft revision | part of the one move-only verified `RawBir` consumed by B1 | any A2 error publishes no module/capability; no memory-only `RawBir` |

## Adjacent-Stage Contract

The complete producer surface is [`ir.hpp`](../../../../codegen/lir/ir.hpp).
The parent [A1 importer](../README.md) owns inventory, dispatch, maps,
transaction and draft completion. [Raw core](../../core/README.md) owns typed
storage; the shared [A2 verifier](../../verify/README.md) alone may consume the
finished draft and mint `RawBir`.

After A2, [B1 legalize](../../passes/legalize/README.md) accepts the exact
published `RawBir`. The later [canonical memory pass](../../passes/memory/README.md)
may normalize already-valid memory semantics but cannot repair lossy import,
invent identity, parse compatibility text, or accept an opaque unsupported
placeholder. This sub-boundary publishes no analysis or target product to
either consumer.

## Ordered Behavior

1. The parent validates the complete module and selects a named matrix row.
2. Types, symbols, globals, strings, locals, blocks and result identities are
   predeclared in parent source order before a use is resolved.
3. This boundary validates exact row fields and contributes one typed spec or
   one validation-only result through the parent builder context.
4. Inline/hoisted allocas and every nested operand/index/object order retain
   their parent-selected position. Old/new twins converge on one Raw owner.
5. The parent resolves forward uses/fixups with all other families and alone
   finishes the complete module draft.
6. A2 consumes that exact draft once; no memory-only phase or retry exists.

## Invariants

- One parent row, one core owner, one parent transaction, and one full A2 gate;
  this document owns none of those mechanisms independently.
- All semantic IDs and ordinary value edges use the common module/function
  identity model. Source IDs/maps are private origin/translation evidence.
- Load/store/GEP twins converge before publication. Compatibility strings and
  initializer text cannot become semantic address or relocation authority.
- Memory semantics remain target-independent and unallocated. No folding,
  scalarization, provenance inference, target layout, ABI, frame, or machine
  fact enters Raw.
- Failure is module-transactional. No placeholder, successful prefix, local
  graph, partial draft, fixup table, analysis cache, or capability escapes.

## Verification and Publication

This boundary may perform row-local validation but owns no verifier profile and
cannot mint a stage token. The parent completes one `ModuleDraft`; only
`verify_and_publish_raw(ModuleDraft&&)` in the full shared A2 gate checks exact
revision, ownership, order, def-use, type/descriptor validity, terminators,
globals/initializers, unresolved state, and absence of target/allocation facts.

The current checked-in foundation builder adapter is not proof of this target
publication contract. A diagnostic-only report cannot bless a draft. Success
publishes the whole module once; every failure publishes nothing.

## Failure and Diagnostics

Failures retain the parent import phase and exact module/function/block/
instruction/field site. They distinguish malformed current input, missing Raw
container, missing parent wiring, invalid/duplicate identity, type/use/order
conflict, unreceivable compatibility payload, builder failure, and unchanged
A2 publication failure.

Before mutation, independent safe validation errors may accumulate. After a
spec contribution begins, any poisoning error destroys all unpublished parent
state. An unsupported valid row is stable fail-closed evidence only, not
receiving coverage and not authority to emit an opaque node.

## Analysis and Invalidation

This boundary publishes no provenance, alias, memory-effect, object-layout,
CFG, dominance, liveness, or call-graph analysis. Private translation maps and
parity observations die with the parent transaction. B1 begins from the exact
published Raw revision without inherited mutable A1 analysis state; later
analyses bind to that revision and follow their own invalidation contracts.

## Target and ABI Rules

Only target-independent current semantic facts may enter Raw. This boundary
does not read `target_profile` or rendered `data_layout` as semantic input, and
does not select/fold addresses, derive target layout, choose relocation/TLS
forms, classify ABI values, assign registers/stack slots, lay out a frame,
lower to machine operations, or prepare MIR/emission.

Exact current alignment/count/volatility/type facts are semantic only where a
named row carries them. Missing desired target/access fields are not inferred
from text, uses, symbols, tests, or downstream behavior.

## Implementation State

Implementation is absent for this sub-boundary. Backend
[`CMakeLists.txt`](../../../CMakeLists.txt) excludes every nested
`bir/lir_to_bir/*.cpp`, including the files beside this document. The
build-included top-level importer at `10d70b872` rejects memory instructions,
globals, strings, stack objects, hoisted allocas, type declarations, intrinsic
requirements, specializations and related module state.

Nested source presence and this target contract are design/migration evidence,
not checked-in receipt. A family becomes implemented only when its parent
dispatch case, typed core owner, builder/view wiring, full A2 rules, build
inclusion and neighboring positive/negative tests land together.

## Proof Requirements

- require the exact metadata spine, core-first headings and applicable detail
  headings in order;
- resolve all relative links and verify build-exclusion evidence;
- require each of the 11 current instruction rows exactly once in this input
  matrix, each with one same-named parent binding and one typed Raw owner;
- require the applicable metadata rows for stack objects/allocas, identities,
  operands, types, values, globals, initializers, strings, requirement parity,
  source order and publication;
- require explicit no-independent-dispatch/storage/verifier/publication and
  parent-transaction/full-A2 clauses;
- require every input/output row to name identity/revision/order,
  optional/error behavior, stable failure and positive plus malformed/neighbor
  proof obligations;
- reject completion for scalarization/folding, text identity, target/ABI/
  allocation/MIR authority, opaque placeholders, partial drafts, or any claim
  that build-excluded files are implementation.

## Open Questions

No open question authorizes an adjacent-owner edit. If the parent importer,
core, A2 verifier, B1 or canonical memory contract cannot accept an exact row,
record the named coordinated-boundary seam in `todo.md`; do not create a second
authority here.

## Review Checklist

- [x] Metadata spine, core-first sections and applicable detail order are exact.
- [x] All 11 current instruction rows and applicable metadata families bind uniquely to parent rows and one Raw owner.
- [x] Input and output matrices state identities, order, optionals/errors, failure and neighboring proof.
- [x] The top-level importer remains the sole dispatcher/private A1 transaction.
- [x] Core remains the sole storage owner and A2 the sole publication gate.
- [x] Build-excluded implementation truth is explicit and design is not receipt.
- [x] No scalarization, folding, target layout, ABI, allocation, MIR/emission or text identity enters Raw.
- [x] Unsupported valid input, opaque placeholders and partial publication are rejected as coverage.
- [x] B1 and the later memory pass can consume only fully verified, already-valid Raw semantics.
