# BIR Phase A Import/Raw Document Convergence

Status: Closed
Type: Documentation-only architecture convergence
Phase Owner: A — import and Raw publication
Predecessor: external typed LIR contract
Successor: `ideas/open/736_bir_phase_b_canonical_document_convergence.md`
Deferred Implementation Consumer: `ideas/open/734_lir_to_new_bir_container_completeness.md`, inactive until this idea is accepted

## Goal

Converge the phase-A Markdown contract, in `A1 -> A2` order, from the complete
current typed LIR surface through private `ModuleDraft` construction, full
Draft/Raw verification, and atomic publication of one move-only, verified,
target-independent `RawBir` accepted by phase B.

## Why This Exists

Current LIR is the immutable external source authority, while the checked-in
new-BIR containers/importer cover only a bounded subset. Documentation must
classify every current source fact without redirecting receiving gaps into LIR,
confusing build-excluded design files with implementation, or treating an
unsupported diagnostic as lossless receipt.

## Scope and Exact Owner Order

1. `A1`: `src/backend/bir/lir_to_bir/README.md`, then its build-excluded
   `src/backend/bir/lir_to_bir/memory/README.md` sub-boundary.
2. `A2`: `src/backend/bir/core/README.md`, then the shared Draft/Raw clauses of
   `src/backend/bir/verify/README.md`.
3. Audit, without reassigning, the applicable root, analysis-framework,
   diagnostics, compatibility, coverage-ledger, and review-template clauses.

The owned edit set is the three phase-A documents named in items 1-2; shared
owners are adjacency/verifier evidence and require a coordinated boundary
packet if their own text must change.

## Uniform Document and Review Contract

Each owned document must carry `Contract-Status: scaffold | under-review |
accepted`, `Implementation-Status: absent | partial | complete`, `Kind`,
`Phase-ID` (or `Applies-To` for a cross-cutting document), `Upstream`,
`Downstream`, `Owner-Path`, and `Last-Reconciled-Commit`. Its core-first order
is `Purpose`, `Owns`, `Does Not Own`, `Inputs`, `Outputs`, and
`Adjacent-Stage Contract`, followed where applicable by ordered behavior,
invariants, verification/publication, failure/diagnostics,
analysis/invalidation, target/ABI rules, implementation state, proof, open
questions, and review checklist.

For every document, review actual content rather than headings; detect and
index missing real owners/placeholders; and produce:

- an exhaustive input matrix naming producer, revision/target key, stable
  identities, validation state, optional/error forms, and exact receiving
  field/rule; and
- an exhaustive output matrix naming the exact consumer, revision/target
  binding, verifier/publication gate, failure behavior, invalidated analyses,
  and the consumer clause proving acceptance.

Failure is transactional: no rejected candidate, partial revision, stale
analysis, fixup map, or readiness token escapes. Semantic identity may not be
inferred from names, pointers, vector positions, render order, or dense
analysis indices. Every implementation-status statement must be checked
against build inclusion, checked-in storage/wiring, verifier reachability, and
proof—not copied from a status label.

## Exhaustive Current `LirInst` Intake Contract

Every row below must expand in the accepted phase-A input matrix to the exact
current fields and their authority class (semantic, compatibility mirror,
producer cache/index, or validation evidence), the importer rule, one current
disposition, and positive plus malformed/neighboring proof. `Raw verifier`
means the full A2 gate; all failures reject the whole module without partial
publication.

| Alternative | Current source authority | Required Raw destination | Current disposition | Verifier obligation | Stable failure |
|---|---|---|---|---|---|
| `LirInst::LirConstInt` | result, `TypeSpec`, `long long` value | typed `ConstantId` and result binding | container+wiring missing | exact type/value/result | reject unrepresentable value; no truncation |
| `LirInst::LirConstFloat` | result, `TypeSpec`, `double` value | typed floating `ConstantId` and result binding | container+wiring missing | exact bit/value/type/result | reject; no textual reconstruction |
| `LirInst::LirLoad` | result/type/pointer IDs | typed `Load` edges | container+wiring missing | ID, pointer/value type, uniqueness | reject invalid use/type |
| `LirInst::LirStore` | pointer/value IDs and type | typed `Store` edges | container+wiring missing | both uses and stored type | reject missing/foreign IDs |
| `LirInst::LirBinary` | result/type/lhs/rhs and integer opcode | shared typed binary/unary payload | container+wiring missing | closed opcode, arity, types, uses | reject unknown opcode |
| `LirInst::LirCast` | result/from/to/operand | shared typed `Cast` | container+wiring missing | exact types and identities | reject without rendered-text inference |
| `LirInst::LirCmp` | result/predicate/lhs/rhs | shared typed `Compare` | container+wiring missing | predicate domain, operands, boolean result | reject invalid predicate |
| `LirInst::LirCall` | result/return/direct name or indirect pointer/ordered args | typed callee, signature, result, args | container+wiring missing | coherent callee and full signature | reject name invention/partial graph |
| `LirInst::LirGep` | result/base type/pointer/ordered index IDs | typed `GetElementPtr` path | container+wiring missing | base/result and ordered indices | reject invalid path; no folding |
| `LirInst::LirSelect` | result/type/condition/arms | typed `Select` | container+wiring missing | boolean condition and equal types | reject invalid identity; no fusion |
| `LirInst::LirIntrinsic` | result/name/ordered args | typed target-independent intrinsic and edges | container+wiring missing | registry, arity, type, result | reject unknown input explicitly |
| `LirInst::LirInlineAsm` | result, asm/constraint strings, ordered operand IDs | ordinary edges plus opaque strings | container+wiring missing | edges and byte/order preservation | reject; never parse assembly |
| `LirInst::LirMemcpyOp` | typed dst/src/size and volatility | typed `Memcpy` | container+wiring missing | kinds, types, dynamic size, volatility | reject; no scalarization |
| `LirInst::LirVaStartOp` | typed `ap_ptr` | typed `VaStart` | container+wiring missing | pointer use/effect | reject malformed use |
| `LirInst::LirVaEndOp` | typed `ap_ptr` | typed `VaEnd` | container+wiring missing | pointer use/effect | reject malformed use |
| `LirInst::LirVaCopyOp` | typed destination/source pointers | typed `VaCopy` | container+wiring missing | ordered pointer uses/effect | reject without ABI interpretation |
| `LirInst::LirStackSaveOp` | typed result operand | typed `StackSave` and result | container+wiring missing | one pointer-like result and order | reject; no frame location |
| `LirInst::LirStackRestoreOp` | typed saved-pointer operand | typed `StackRestore` | container+wiring missing | matching use and order | reject invalid use |
| `LirInst::LirAbsOp` | result/argument/type | typed target-independent `Abs` | container+wiring missing | identities and exact integer type | reject; invent no flags |
| `LirInst::LirIndirectBrOp` | address operand and ordered label strings | sole `IndirectJumpTerm` owner | container+wiring missing; second instruction is stale | last/sentinel rule, targets/order | reject with no hidden edge |
| `LirInst::LirExtractValueOp` | result/aggregate/type/index | typed `ExtractValue` | container+wiring missing | shape/index/result/IDs | reject out-of-range |
| `LirInst::LirInsertValueOp` | result/aggregate/element/types/index | typed `InsertValue` | container+wiring missing | aggregate/element/index/result | reject; no leaf expansion |
| `LirInst::LirLoadOp` | result/pointer operands and `LirTypeRef` | same typed `Load` | container+wiring missing | kinds/type/result | reject semantic `RawText` |
| `LirInst::LirStoreOp` | value/pointer operands and `LirTypeRef` | same typed `Store` | container+wiring missing | uses and exact type | reject without text decoding |
| `LirInst::LirMemsetOp` | typed dst/byte/size and volatility | typed `Memset` | container+wiring missing | kinds/widths/order/volatility | reject; no scalarization |
| `LirInst::LirCastOp` | result/operand/`LirCastKind`/types | same typed `Cast` | container+wiring missing | closed cast-kind and type handling | reject generic-success arm |
| `LirInst::LirGepOp` | result/pointer/element type/inbounds/ordered index strings | same typed `GetElementPtr` | container+wiring missing | base/result/type/inbounds/path | reject inadmissible current path |
| `LirInst::LirCallOp` | result/return/callee/link ID/signature/args/types/extensions/mirrors | same typed `Call` | container+wiring missing | structured fields win; parity checked | reject incoherence atomically |
| `LirInst::LirBinOp` | result/lhs/rhs/opcode ref/type ref | same binary/unary payload | container+wiring missing | closed opcode/arity/type/uses | reject untyped/unknown ref |
| `LirInst::LirCmpOp` | result/lhs/rhs/float flag/predicate/type | same typed `Compare` | container+wiring missing | domain/predicate/types | reject inferred predicate |
| `LirInst::LirPhiOp` | result/type/ordered `(value,label)` pairs | typed `Phi` on exact CFG edge IDs | container+wiring missing | one incoming per edge and multiplicity | reject ambiguous labels |
| `LirInst::LirSelectOp` | result/type/condition/arms | same typed `Select` | container+wiring missing | condition/types/ordinary IDs | reject raw/unresolved operand |
| `LirInst::LirInsertElementOp` | result/vector/element/index/types | typed `InsertElement` | container+wiring missing | lane/index/types/result | reject malformed lane/token |
| `LirInst::LirExtractElementOp` | result/vector/index/types | typed `ExtractElement` | container+wiring missing | vector/index/result | reject malformed lane |
| `LirInst::LirShuffleVectorOp` | result/two vectors/mask/type refs | typed `ShuffleVector` | container+wiring missing | input/mask/result shapes | reject; no target selection |
| `LirInst::LirVaArgOp` | result/ap-pointer/result type | typed `VaArg` | container+wiring missing | use/result/type | reject; no ABI expansion |
| `LirInst::LirAllocaOp` | result/element/optional count/alignment | typed semantic local/`Alloca` | container+wiring missing | type/count/alignment/order | reject; no frame placement |
| `LirInst::LirInlineAsmOp` | bindings(value/type/role/index), original texts, clobbers, effects, optional `insn_r`, mirrors | ordinary edges, opaque payload, typed role/index attachment | generic edges/payload proved; role/index container+wiring missing | order/roles/indices/types/read-write/bytes | reject interpreted `insn_r` or partial receipt |

Old/new twins remain separate source rows but converge on one semantic BIR
opcode. Closed dispatch tripwires must enforce exactly 38 alternatives and a
maintained field-family checklist; no catch-all visitor arm is acceptance.

## Exhaustive Current Terminator Intake Contract

| Alternative | Current source authority | Raw destination | Disposition | Verifier and failure contract |
|---|---|---|---|---|
| `LirTerminator::LirBr` | target label | `JumpTerm(BlockId)` | proved for unique label | sole successor; missing target rejects module |
| `LirTerminator::LirCondBr` | condition and ordered true/false labels | checked-in `CondJumpTerm` | wiring missing | boolean value, both targets, parallel edges; reject invalid facts |
| `LirTerminator::LirRet` | optional value/type strings | checked-in `ReturnTerm` | void proved; non-void wiring missing | function-result parity; reject missing/type-invalid value |
| `LirTerminator::LirSwitch` | selector/type/default/ordered cases | typed `SwitchTerm` | container+wiring missing | cases, targets, duplicates, multiplicity; reject conflict |
| `LirTerminator::LirIndirectBr` | structured address ID and ordered block IDs | typed `IndirectJumpTerm` | container+wiring missing | ownership/order and sentinel reconciliation; reject invalid target |
| `LirTerminator::LirUnreachable` | empty typed alternative | `UnreachableTerm` | proved | no successor/fallthrough; exactly one terminator |

## Exhaustive Metadata-Family Intake Contract

Each named family must have its own accepted matrix row with exact source
authority, authority class, destination or validation-only non-destination,
import rule, current disposition, verifier, stable failure, and positive plus
malformed/neighbor proof:

| Family key | Exact current source authority | Destination | Current disposition | Verifier and stable failure obligation |
|---|---|---|---|---|
| `module-context` | `LirModule::target_profile`, rendered `data_layout` | validation/origin/parity-only; no semantic Raw destination | stale if described as imported semantics; parity wiring missing | reject any influence on Raw facts; C1 independently selects `TargetProfile`, C2 derives layout |
| `stable-identities` | `LirValueId`, `LirBlockId`, `LirStackSlotId`, `LirGlobalId`, `LinkNameId`, `StructNameId`, invalid sentinels | typed BIR IDs plus origin mapping | block/asm slice partial; complete container+wiring missing | validate scope, uniqueness, sentinels, forward refs; reject name replacement |
| `operand-kinds` | `LirOperand::{text,kind}` with SSA/global/label/immediate/special/raw alternatives | closed typed operand/constant/symbol/block destination or explicit compatibility/error form | container+wiring missing outside bounded asm; absent-classification prose stale | every kind imports or fails; reject `RawText` semantic identity |
| `type-system` | `TypeSpec`, `LirTypeRef` kind/width/name, opcode/predicate refs, `LirCastKind`, `LirStructDecl/Field` | interned type graph, typed discriminants and named aggregates | bounded scalar/pointer core partial; complete container+wiring missing | validate interning, kinds, widths, recursion and mirror parity; reject invented fields |
| `functions-signatures` | function/link ID, flags, returns/params, variadic/void-list forms, structured refs, `signature_text` | typed function/signature/symbol/ordered parameter IDs; text parity-only | void/zero-param shell partial; remainder container+wiring missing | preserve all optionals; verify declaration/body coherence; reject text-derived identity |
| `blocks-cfg-order` | block ID/label/ordered insts/terminator, function block vector and explicit entry | typed block/order/terminator/entry and debug label | shell partial; non-first entry and complete CFG wiring missing | entry is explicit ID; verify exact order/target ownership; reject positional policy |
| `values-def-use` | all result/use IDs and typed operands, parameters, phi incoming, asm bindings | typed ordinary definitions and exact use edges | bounded asm partial; general container+wiring missing | predeclare; verify uniqueness/type/owner/forward uses; reject hidden value systems |
| `stack-objects-allocas` | `LirStackObject` fields/order and inline/hoisted `LirAllocaOp` order | semantic local objects and `Alloca` operations | container+wiring missing | verify identity/type/alignment/count/order; reject frame placement or dropping |
| `globals-objects` | global/link IDs, type/internal/const/linkage/qualifier mirrors, alignment, extern state | typed symbol/global/object/declaration | container+wiring missing; imagined TLS/section source-gap prose stale | preserve flags/identity/order; reject conflicts without inventing metadata |
| `initializers` | `LirGlobal::init_text`, ordered initializer function link IDs | typed initializer owner with exact compatibility-payload classification | container+wiring missing; future-tree prerequisite prose stale | verify referenced IDs; reject loss or text-parsed invented topology |
| `strings` | ordered pool name/raw bytes/length and module cache/counter | typed string/data object with exact bytes, length and symbol identity | container+wiring missing; absent-row prose stale | validate content/length/name/cache parity; reject hash-order identity |
| `externs` | ordered `LirExternDecl`/`ExternDeclInfo` and link/name dedup maps | typed external symbol/declaration/signature; maps validation-only | container+wiring missing; map-as-authority prose stale | merge coherent IDs, validate maps; reject conflict transactionally |
| `specializations` | ordered key/origin/mangled name/mangled link ID | typed specialization/origin record linked to ordinary symbol | container+wiring missing | preserve order/identities; reject parity mismatch or dangling symbol |
| `intrinsic-requirements` | all `need_va_*`, `need_mem*`, `need_stack*`, `need_abs`, `need_ptrmask`, `prefer_semantic_va_ops` | validation-only typed requirement/parity result | wiring missing; second instruction authority stale | cross-check operations/declarations; reject inconsistency; never synthesize instructions |
| `inline-asm-metadata` | bindings, roles, original texts, clobbers, effects, optional `LirInlineAsmInsnRMetadata` | ordinary edges, typed role/index attachment, opaque payload; `insn_r` audit-only | payload/edges proved; role/index container+wiring missing | verify bytes/order/roles/indices/types/read-write; reject parsing/allocation |
| `producer-indexes-caches` | extern/struct/string/intern/name/layout indexes, observations and counters | validation/import report only unless ordered semantic row names destination | semantic-input claims stale; parity wiring missing | compare deterministically to ordered authority; reject pointer/hash/storage identity publication |
| `source-order-origin` | module/function/block/instruction vectors; ordered args/cases/indices/bindings; labels/sites | typed deterministic orders plus non-authoritative origin/debug attachments | function/block/inst partial; remaining container+wiring missing | preserve every nested order/source coordinate; reject hash iteration leakage |
| `module-publication` | complete `LirModule`, public verification, malformed kind and all alternatives/families above | one private `ModuleDraft` consumed by full Raw gate | bounded foundation proved; full container+wiring missing | success mints exactly one verified revision; any row failure publishes no draft, fixup or capability |

## Inline Assembly and Deferred Implementation Boundary

The current LIR carrier already suffices. `LirInlineAsmOp` retains ordinary
values, exact types, `Input`/`Output`/`ReadWrite` role, constraint index,
original constraint text, and byte-preservable opaque asm text. No LIR schema
exception, asm-only value model, tie/group architecture, parsing, allocation,
or projection is justified. The gap is Raw-BIR ownership and importer receipt
of role/index facts; optional `insn_r` remains audit-only.

Idea 734 is a deferred inactive implementation consumer only after this idea's
acceptance. It receives the accepted container/import/verifier contract but no
authority to edit LIR, invent producer forms, canonicalize, select targets,
allocate, build MIR, emit, parse assembly, or claim valid rows complete through
unsupported diagnostics. This idea performs no implementation and does not
activate 734.

## Non-Goals

- Any LIR, C++, test, build, runtime, target, allocation, MIR, or emission edit.
- Canonicalization or target interpretation in Raw BIR.
- Reassigning shared verifier/analysis/diagnostic authority.
- Activating idea 734 or draft idea 733.

## Acceptance and Closure Criteria

- All three owned documents conform in A1/A2 order and truthfully distinguish
  checked-in/build-included behavior from design or placeholder claims.
- The durable matrices contain 38 individually named instruction rows, six
  terminator rows, and 18 individually named metadata-family rows with all
  authority/destination/disposition/verifier/failure/proof fields.
- Raw publication is lossless, deterministic, target-independent,
  unallocated, exact-revision, verifier-gated, and failure-atomic.
- Phase B is shown to accept only the move-only verified `RawBir`, never draft
  state, importer maps, unsupported valid rows, or hidden side tables.
- Closure records edited/created owners, matrices, stale claims corrected,
  implementation-truth findings, adjacency proof, unresolved external owners,
  and confirmation that idea 734 remained deferred and LIR unchanged.

## Completion Note

Accepted and closed after the supervisor-approved documentation proof in
`2ec50b3e`. The completed route comprises importer convergence `10d70b872`,
the subordinate memory boundary `060a32c78`, Raw core receiving ownership
`e759322a`, the read-only shared audit `237afcdf`, coordinated-boundary
authorization `ba1dcab8`, and the accepted shared-document repair `65a20c2d`.

The three phase-A owners and their coordinated shared boundary now prove the
exact 38 instruction, six terminator and 18 metadata-family intake and
receiving contract; one private exact-revision `ModuleDraft` reaches the sole
full A2 gate and publishes one move-only, target-independent, unallocated
`RawBir` accepted as B1's immutable input. Implementation truth remains
explicitly partial/absent: this documentation acceptance does not claim the
missing containers, wiring or full gate are implemented. No LIR file or idea
734 changed, and idea 734 remains a deferred inactive implementation consumer.

## Reviewer Reject Signals

- Any implementation/code/test/build edit, LIR edit, mixed phase ownership, or
  activation of 734/733.
- A missing/catch-all 38/6/18 row, status-only/heading-only edit, or assertion
  that phase B accepts output without its exact consumer clause and gate.
- A name-, pointer-, position-, rendered-text-, cache-, or dense-index identity
  seam; stale analysis/product key; partial publication; or optional/error form
  silently dropped.
- `target_profile` or `data_layout` entering Raw semantics, C1 authority being
  pulled into phase A, or inline asm being parsed/allocated/redesigned in LIR.
- Unsupported diagnostics, helper renames, classification-only changes,
  expected-output weakening, supported-to-unsupported changes, allowlists,
  named-test matchers, or testcase-shaped shortcuts claimed as coverage.
- An implementation-status claim not reconciled to current code/build/verifier
  truth, or the old receiving failure preserved behind a renamed abstraction.
