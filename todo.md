# Current Packet

Status: Active
Source Idea Path: ideas/open/732_bir_stage_document_convergence_umbrella.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Create the ordered A-through-F documentation child queue

## Just Finished

- Completed plan Step 2 as a docs-only phase-A intake packet. The inventory
  below accounts for all 38 current `LirInst` alternatives, all six current
  `LirTerminator` alternatives, and every required current metadata family.
- `src/codegen/lir/ir.hpp` and its public model subheaders are the unchanged
  producer authority. This matrix describes only facts that exist there; a
  desired backend feature absent from current LIR is not a phase-A input and
  must not be used to reopen or redesign LIR.
- Current implementation evidence is the build-included top-level
  `src/backend/bir/lir_to_bir.cpp` plus the checked-in new-BIR core. Nested
  `src/backend/bir/lir_to_bir/*.cpp` files are deliberately build-excluded and
  are design/legacy evidence, not receiving coverage.

### Phase-A disposition vocabulary

- **already-proved coverage**: the build-included importer constructs the
  named current typed Raw-BIR form and the current publication verifier checks
  the bounded route.
- **stale documentation**: phase-A prose calls an already-present LIR fact a
  producer/source gap, asks LIR to acquire a form outside the current surface,
  or treats a producer cache/rendering mirror as a second semantic input.
- **missing new-BIR receiving container**: current LIR has the fact but the
  checked-in Raw-BIR schema has no typed owner for it.
- **missing importer wiring**: a checked-in Raw-BIR owner exists, but the
  build-included importer rejects, ignores, narrows, or fails to preserve the
  current fact.
- **both missing container+wiring**: neither checked-in typed storage nor the
  build-included importer receives the current fact. A row can additionally
  flag stale documentation, but valid current input is not complete until its
  container and wiring disposition is resolved.

The required destinations below are semantic contract names from the accepted
phase-A design, not implementation instructions. Every accepted Child-A row
must bind the current LIR fields to exactly one typed Raw-BIR owner, or state
explicitly that a cache/index is validation-only and has no published semantic
destination.

### Exhaustive `LirInst` source/receiving matrix

| Variant | Exact current LIR authority | Required typed Raw-BIR destination | Current disposition | Child-A verifier and failure obligation |
|---|---|---|---|---|
| `LirInst::LirConstInt` | `result`, `TypeSpec type`, `long long value` | interned typed `ConstantId` plus the ordinary result-value binding | both missing container+wiring; prose demanding non-current arbitrary-width payload is stale documentation | prove exact current type/value/result preservation; malformed or unrepresentable current input rejects the module, never truncates or widens silently |
| `LirInst::LirConstFloat` | `result`, `TypeSpec type`, `double value` | interned typed floating `ConstantId` plus result binding | both missing container+wiring; prose demanding non-current f80/f128 bit carriers is stale documentation | prove bit/value/type/result preservation for the current field; no textual reconstruction or partial publication |
| `LirInst::LirLoad` | `result`, `type`, `ptr` IDs | typed `Load` payload with ordinary operand/result edges | both missing container+wiring; extra access attributes absent from this current variant are not phase-A inputs | verify IDs, pointer/value type, result uniqueness, and ownership; reject invalid use/type without publishing |
| `LirInst::LirStore` | `ptr`, `val` IDs and `type` | typed `Store` payload with ordinary operand edges | both missing container+wiring; non-current attributes are stale source-gap scope | verify both uses and stored type; reject missing/foreign IDs transactionally |
| `LirInst::LirBinary` | result/type/lhs/rhs IDs and current integer `op` field | the single typed Raw binary/unary semantic payload used by its newer twin | both missing container+wiring; docs must classify the existing `op` fact rather than request a new variant | verifier requires one admitted current opcode mapping, arity, types, and defined uses; unknown/malformed values fail at this row |
| `LirInst::LirCast` | result, `from_type`, `to_type`, operand | the single typed `Cast` payload used by `LirCastOp` | both missing container+wiring | verify source/destination types and result/use identity; reject an unclassifiable current cast without guessing from rendered text |
| `LirInst::LirCmp` | result, current predicate field, lhs/rhs IDs | the single typed `Compare` payload used by `LirCmpOp` | both missing container+wiring | verify predicate/domain, operand compatibility, and boolean result; fail closed on invalid current predicate |
| `LirInst::LirCall` | result, return type, direct name/indirect pointer, ordered argument IDs | typed `Call` payload with symbol-or-value callee, signature/result, and ordered ordinary arguments | both missing container+wiring; desired effects/bundles absent here are not current facts | validate mutually coherent direct/indirect identity, signature/result and every argument; no name-based semantic invention or partial graph |
| `LirInst::LirGep` | result, base type/pointer, ordered index value IDs | typed `GetElementPtr` payload with ordered typed path | both missing container+wiring | verify base/result and every ordered index identity/type; invalid path rejects rather than folds |
| `LirInst::LirSelect` | result/type, condition and two value IDs | typed `Select` payload | both missing container+wiring | require boolean condition, equal arm/result types and valid IDs; no compare fusion |
| `LirInst::LirIntrinsic` | result ID, intrinsic `name`, ordered argument IDs | typed target-independent `Intrinsic` registry payload and ordinary edges | both missing container+wiring; flags/signatures not present in this variant are not presumed | require a lossless current-name registry disposition and exact arity/type/result rule; unknown current input rejects explicitly |
| `LirInst::LirInlineAsm` | result ID, asm string, constraint string, ordered operand IDs | core `InlineAsm` with generic ordinary operands/results and opaque exact strings | both missing container+wiring | validate every ordinary edge and preserve exact bytes/order; never parse assembly or create a parallel value model |
| `LirInst::LirMemcpyOp` | typed dst/src/size operands and volatility | typed semantic `Memcpy` payload | both missing container+wiring | verify operand kinds/types, dynamic size and volatility; no scalarization or access guessing |
| `LirInst::LirVaStartOp` | typed `ap_ptr` operand | typed `VaStart` semantic payload | both missing container+wiring | verify va-list pointer use and effect; malformed input fails module publication |
| `LirInst::LirVaEndOp` | typed `ap_ptr` operand | typed `VaEnd` semantic payload | both missing container+wiring | verify va-list pointer use and effect; malformed input fails module publication |
| `LirInst::LirVaCopyOp` | typed destination/source pointers | typed `VaCopy` semantic payload | both missing container+wiring | verify ordered source/destination pointer uses and effect; reject invalid alias/type facts without ABI interpretation |
| `LirInst::LirStackSaveOp` | typed ordinary result operand | typed `StackSave` semantic payload and ordinary result | both missing container+wiring | verify one valid pointer-like result definition and ordering; do not assign a frame location |
| `LirInst::LirStackRestoreOp` | typed saved-pointer operand | typed `StackRestore` semantic payload | both missing container+wiring | verify matching valid use and ordering; reject invalid use transactionally |
| `LirInst::LirAbsOp` | result/argument operands and typed integer reference | typed target-independent `Abs` semantic payload | both missing container+wiring | verify result/argument identity and exact current integer type; do not invent overflow flags |
| `LirInst::LirIndirectBrOp` | typed address operand plus ordered target-label strings | the one `IndirectJumpTerm` CFG owner, with this instruction-form reconciled as the current producer carrier | both missing container+wiring; documentation proposing a second ordinary instruction is stale | require last-position/sentinel reconciliation, unique resolved targets and exact order; failure leaves no hidden CFG edge or published module |
| `LirInst::LirExtractValueOp` | result/aggregate operands, aggregate type and field index | typed aggregate `ExtractValue` payload | both missing container+wiring | verify aggregate shape/index/result type and ordinary IDs; reject out-of-range or text-only invention |
| `LirInst::LirInsertValueOp` | result/aggregate/element operands, both types and field index | typed aggregate `InsertValue` payload | both missing container+wiring | verify aggregate/element/index/result consistency, including current special tokens; no leaf expansion |
| `LirInst::LirLoadOp` | result/pointer operands and `LirTypeRef` | same typed `Load` payload as `LirLoad` | both missing container+wiring; README requests for absent volatile/alignment/address-space fields are stale for this intake | verify admitted operand kinds, type and result; reject `RawText` where semantic identity is required |
| `LirInst::LirStoreOp` | value/pointer operands and `LirTypeRef` | same typed `Store` payload as `LirStore` | both missing container+wiring; non-current access fields are outside the row | verify both uses and exact current type; fail closed without decoding rendered fragments |
| `LirInst::LirMemsetOp` | typed dst/byte/size operands and volatility | typed semantic `Memset` payload | both missing container+wiring | verify operand kinds/widths, order and volatility; no scalarization |
| `LirInst::LirCastOp` | result/operand, typed `LirCastKind`, from/to refs | same typed `Cast` payload as `LirCast` | both missing container+wiring | closed handling for every current cast kind, exact type/result verification, no generic-success arm |
| `LirInst::LirGepOp` | result/pointer, element type, inbounds, ordered current index strings | same typed `GetElementPtr` payload as `LirGep`, retaining exact current path facts | both missing container+wiring; docs must not make a future carrier a prerequisite to inventorying this current row | verify base/result, element type, `inbounds`, path order and admissible current index representation; no folding |
| `LirInst::LirCallOp` | result/return/callee, direct `LinkNameId`, optional structured signature, ordered `LirCallArg`s/type refs, extension attrs, plus compatibility strings | typed `Call` payload: `SymbolId` or ordinary callee, function type, ordered args, result, current attributes | both missing container+wiring; claims that already-present signature/argument/extension fields are source gaps are stale | structured fields win; parity-only strings cannot create identity. Verify all present optional/error forms and reject incoherence atomically |
| `LirInst::LirBinOp` | result/lhs/rhs operands, typed opcode ref and type ref | same typed binary/unary payload as `LirBinary` | both missing container+wiring | closed typed-opcode dispatch, arity/type/use/result checks, and exact failure for an untyped/unknown ref |
| `LirInst::LirCmpOp` | result/lhs/rhs, float-domain flag, predicate ref, type ref | same typed `Compare` payload as `LirCmp` | both missing container+wiring | verify predicate belongs to domain and operands/result type; no inferred predicate |
| `LirInst::LirPhiOp` | result/type plus ordered current `(value,label)` incoming pairs | typed `Phi` payload keyed to exact derived CFG edge identities | both missing container+wiring; requiring a future source shape instead of documenting current input is stale | verify one typed incoming per required edge, order/multiplicity and no extras; ambiguous current labels fail without predecessor guessing |
| `LirInst::LirSelectOp` | result/type/condition/true/false operands | same typed `Select` payload as `LirSelect` | both missing container+wiring | verify condition, arm/result types and ordinary IDs; reject raw/unresolved operands |
| `LirInst::LirInsertElementOp` | result/vector/element/index operands and vector/element types | typed `InsertElement` payload | both missing container+wiring | verify lane/index/type/result and current poison/undef tokens; no SIMD selection |
| `LirInst::LirExtractElementOp` | result/vector/index operands plus vector/index types | typed `ExtractElement` payload | both missing container+wiring | verify vector/index/result facts and ordinary identities; reject malformed lane access |
| `LirInst::LirShuffleVectorOp` | result, two vectors, mask operands and vector/mask types | typed `ShuffleVector` payload | both missing container+wiring | verify two input shapes, mask shape/content representation and result; no target shuffle selection |
| `LirInst::LirVaArgOp` | result/ap-pointer operands and result type | typed `VaArg` semantic payload | both missing container+wiring | verify va-list use/result definition/type; preserve semantics without ABI expansion |
| `LirInst::LirAllocaOp` | result, element type, optional dynamic count, alignment | typed semantic `Alloca`/local-storage payload with ordinary result | both missing container+wiring | verify element/count/alignment/result and static/dynamic form; no frame offset or lifetime inference |
| `LirInst::LirInlineAsmOp` | ordinary input/result bindings (`value`, `type`, `role`, `constraint_index`), original asm/constraint text, ordered clobbers, side effects, optional `insn_r`, plus non-authoritative rendering mirrors | core `InlineAsm` generic value edges and opaque payload, plus a typed position/role requirement attachment for the existing binding facts; `insn_r` remains non-authoritative audit metadata | already-proved coverage for generic inputs/results, original text, clobbers and side effects; both missing container+wiring for retained role/constraint-index facts; rendered mirrors and `insn_r` semantic use are stale | verify binding order, roles, indices, types, read/write pairs and exact payload bytes; reject malformed input or interpreted `insn_r` without partial publication |

The old/new twins are input variants, not permission to create duplicate BIR
opcodes. Child A must assign each twin its own source row and the same semantic
destination where applicable. Unsupported diagnostics prove fail-closed
behavior only; they do not convert any valid current row above into receiving
coverage.

### Exhaustive `LirTerminator` source/receiving matrix

| Variant | Exact current LIR authority | Required typed Raw-BIR destination | Current disposition | Child-A verifier and failure obligation |
|---|---|---|---|---|
| `LirTerminator::LirBr` | `target_label` | `JumpTerm(BlockId)` | already-proved coverage for a uniquely resolved label | verify nonempty unique target and sole-successor authority; missing target rejects the module |
| `LirTerminator::LirCondBr` | condition name plus ordered true/false labels | existing checked-in `CondJumpTerm(ValueId, BlockId, BlockId)` | missing importer wiring; prose calling the current carrier absent is stale documentation | resolve one boolean ordinary value and both targets, preserve equal/parallel edges, reject missing/type-invalid facts |
| `LirTerminator::LirRet` | optional value string and type string | existing checked-in `ReturnTerm(optional<ValueId>)` | already-proved coverage only for void; missing importer wiring for the current non-void alternative; blanket source-gap wording is stale | verify zero/one value against semantic function result; invalid/missing identity rejects without ABI lane expansion |
| `LirTerminator::LirSwitch` | selector name/type, default label and ordered `(value,label)` cases | typed `SwitchTerm` with selector operand, typed case constants and ordered targets | both missing container+wiring | verify selector/cases/targets, duplicate behavior and successor-slot multiplicity; preserve order and reject conflicts |
| `LirTerminator::LirIndirectBr` | structured address `LirValueId` plus ordered `LirBlockId` targets | typed `IndirectJumpTerm` | both missing container+wiring; this existing structured form must not be labeled a source gap | verify valid address, complete ordered target set and ownership; reconcile with `LirIndirectBrOp` only under the documented sentinel rule |
| `LirTerminator::LirUnreachable` | empty typed alternative | `UnreachableTerm` | already-proved coverage | verify no successor/fallthrough and exactly one terminator |

### Required metadata-family source/receiving checklist

| Family key | Exact current LIR authority | Required Raw-BIR destination or validation-only disposition | Current disposition | Child-A acceptance check |
|---|---|---|---|---|
| `module-context` | `LirModule::target_profile`, rendered `data_layout` | validation/origin/parity context only, with an explicit validation-only non-destination; neither field is semantic Raw-BIR storage | stale documentation if either field is described as imported Raw/Canonical semantics; validation/origin/parity wiring is missing | prevent both fields from influencing target-independent Raw semantics; audit and preserve any genuinely target-independent typed LIR fact through its own typed row without parsing rendered `data_layout`; C1 later selects its own exact `TargetProfile` and C2 derives target-layout facts |
| `stable-identities` | `LirValueId`, `LirBlockId`, `LirStackSlotId`, `LirGlobalId`, `LinkNameId`, `StructNameId`; invalid sentinels | typed BIR value/block/local/global/symbol/type-name identities plus origin mapping | only block IDs and bounded inline-asm values are partly wired; complete family is both missing container+wiring | validate scope, uniqueness, invalid sentinels, forward references and no name-based replacement |
| `operand-kinds` | `LirOperand::{text,kind}` with SSA/global/label/immediate/special/raw alternatives | closed typed operand/constant/symbol/block destination selected by kind; raw text only as an explicit compatibility/error form | both missing container+wiring outside bounded inline asm; treating classification as absent is stale | every kind has import-or-fail handling; `RawText` never manufactures semantic identity |
| `type-system` | `TypeSpec`; `LirTypeRef` kind, widths and optional `StructNameId`; binary opcode/predicate refs; `LirCastKind`; `LirStructDecl/Field` | interned `TypeId` graph, typed opcode/predicate/cast discriminants, named aggregate definitions | current core receives only Void/I1/I8/I16/I32/I64/F32/F64/Pointer in the bounded route; complete current family is both missing container+wiring | structured facts win; verify interning, recursive declarations, widths/kinds and mirror parity without requiring imagined source fields |
| `functions-signatures` | `LirFunction` name/link ID, internal/elision/declaration flags, return/params, variadic/void-list flags, structured params/type refs, optional return ref, compatibility `signature_text` | typed `FunctionData`, `FunctionSignature`, symbol/attributes, ordered parameter `ValueId`s; text is parity/origin only | zero-parameter void declaration/definition shell is already proved; all other current facts are both missing container+wiring | preserve every optional flag/param/result/link identity; verify declaration/body coherence and reject text-derived semantics |
| `blocks-cfg-order` | `LirBlock` ID/label/ordered insts/terminator; `LirFunction::blocks` and explicit `entry` | typed `BlockId`, debug/origin label, instruction order, one terminator and explicit entry identity | block/order/label shell is proved; explicit non-first entry and complete CFG are missing importer wiring or the containers named above | entry is the explicit ID, never vector-position policy; verify exact order and target ownership |
| `values-def-use` | result/use IDs and typed operands across variants, parameters, phi incoming and inline-asm bindings | ordinary typed `ValueId`, parameter/constant/instruction-result definitions and exact use edges | bounded inline-asm results/uses are proved; parameters, constants and general instructions are both missing container+wiring | predeclare definitions, verify uniqueness/type/owner and all forward uses; no hidden side value system |
| `stack-objects-allocas` | `LirStackObject` ID/name/type/alignment/VLA; ordered `stack_objects`; inline and hoisted `LirAllocaOp` in `alloca_insts` | typed semantic local/storage object plus `Alloca` operations in exact source order | both missing container+wiring | verify identity/type/alignment/count/order; no frame placement or silent dropping |
| `globals-objects` | `LirGlobal` ID/name/link ID/type/internal/const, linkage/qualifier/type mirrors, alignment and extern state | typed `SymbolId`, `GlobalId`, object/type/linkage/declaration record | both missing container+wiring; source-gap prose about non-current TLS/section suites is stale intake scope | preserve every current flag/identity/order, audit mirrors, reject conflicts without inventing absent metadata |
| `initializers` | `LirGlobal::init_text` plus ordered `initializer_function_link_name_ids` | typed initializer owner retaining every current authoritative ID and exact compatibility payload classification | both missing container+wiring; docs demanding a future recursive producer tree instead of classifying current facts are stale | never parse text to invent topology; verify referenced IDs and exact payload disposition, and fail explicitly if lossless typed receipt cannot be defined |
| `strings` | ordered `LirStringConst::{pool_name,raw_bytes,byte_length}` plus module pool cache/counter | typed string/data object with exact current bytes/spelling classification, length and symbol identity | both missing container+wiring; calling the existing row absent is stale | validate length/name/content and cache parity; IDs follow vector order, never hash order |
| `externs` | ordered `LirExternDecl` and `ExternDeclInfo` name/return type/extension/link ID; link/name dedup maps | typed external function symbol/declaration/signature; maps are validation-only parity indexes | both missing container+wiring; treating maps as a second declaration source is stale | merge only coherent identities, validate maps against ordered declarations, and reject conflicts transactionally |
| `specializations` | ordered `LirSpecEntry` key/origin/mangled name/mangled link ID | typed module specialization/origin record linked to the ordinary symbol | both missing container+wiring | preserve order and all identities; validate mangled ID/name parity and reject dangling symbols |
| `intrinsic-requirements` | all `need_va_*`, `need_mem*`, `need_stack*`, `need_abs`, `need_ptrmask`, `prefer_semantic_va_ops` flags | validation-only typed requirement summary or parity result; never synthesized operations | missing importer wiring; a second semantic instruction container would be stale duplicate authority | cross-check flags against actual operations/declarations and reject inconsistency; do not create instructions from flags |
| `inline-asm-metadata` | `LirInlineAsmValueBinding`, `LirInlineAsmValueRole`, original texts, clobbers, side effects, optional `LirInlineAsmInsnRMetadata` | generic ordinary `ValueId` edges plus typed role/constraint-position attachments and opaque payload; `insn_r` audit-only | bounded payload/edges already proved; role/index attachment is both missing container+wiring | exact order/roles/index/type/read-write checks; preserve bytes and reject any parsed/allocated interpretation |
| `producer-indexes-caches` | extern maps, `struct_decl_index`, `str_pool_map/idx`, intern storage, link/struct name tables, layout observations and ID counters | validation/import report only unless the ordered semantic table names a typed destination | stale documentation if treated as semantic input; parity/consistency wiring is missing | compare to ordered authority deterministically; never publish pointer/storage addresses, hash order, or duplicate entities |
| `source-order-origin` | module vectors; function parameter/block/stack/alloca vectors; block instruction vectors; ordered args/cases/indices/bindings; names/labels/layout-observation site | typed module/function/block orders plus non-authoritative origin/debug attachments | function/block/instruction order and labels are partly proved; module objects and most nested orders are both missing container+wiring | IDs are assigned by declared order, every nested order survives, diagnostics use stable source coordinates, and no hash iteration leaks |
| `module-publication` | complete `LirModule`, public `verify_module`, malformed error kind, and every current alternative/family above | one private `ModuleDraft` consumed by full Raw verifier to mint one `RawBir`; structured import diagnostics remain available on failure | bounded foundation publication is already proved; full schema/verifier/import transaction is both missing container+wiring | success returns exactly one fully verified revision; any row failure returns no draft, partial function, capability, fixup table or mixed state |

### Inline-assembly carrier finding

The evidence-gated LIR exception is **not justified by the current evidence**.
`LirInlineAsmOp` already records each ordinary value through the same
`LirOperand` carrier used elsewhere, its exact `LirTypeRef`, one of
`Input`/`Output`/`ReadWrite`, and a `constraint_index`. It also retains
`original_constraint_text`, so reviewed spellings such as `r`, `=r`, `f`,
`VR`, and any other actually evidenced token can be associated with the
ordinary position/role without adding a new LIR form. `original_asm_text` is a
`std::string` semantic payload and the current importer copies it without
parsing, permitting byte-exact opaque transport (including embedded NUL by
length) to the later assembler.

The receiving gap is in Raw BIR: generic inputs/results and the opaque payload
already land, but current `InlineAsmNode` has no typed owner for the existing
binding role and constraint-index facts. Child A must document that receiving
field and its verifier rule. It must not propose `InlineAsmOperand`, a name
binding table, parsed alternatives, tie/group architecture, allocation,
projection, mnemonic/placeholder/directive parsing, or any LIR edit. Optional
`insn_r` remains non-authoritative evidence and cannot replace opaque text or
ordinary edges.

### Exact Child-A matrix contract

Child A owns only the documentation convergence of
`src/backend/bir/lir_to_bir/README.md`,
`src/backend/bir/lir_to_bir/memory/README.md`, and
`src/backend/bir/core/README.md`, with relevant clauses of the shared verifier,
diagnostic, root, analysis, and boundary owners reviewed but not reassigned.
Its accepted matrix must:

1. contain exactly one individually named row for each of the 38 instruction
   alternatives and six terminator alternatives above, plus every exact family
   key in the metadata checklist; grouping may explain shared destinations but
   may not replace a source row or use “other”, “remaining”, or a generic
   visitor arm;
2. record for every row the exact LIR type and fields, whether each field is
   semantic authority, compatibility mirror, producer index/cache, or
   validation evidence, the single typed Raw-BIR destination (or explicit
   validation-only non-destination), importer rule, one disposition from the
   vocabulary above, verifier owner/rule, stable failure behavior, and positive
   plus malformed/neighboring proof obligation;
3. cover stable identities, revision context, validation/origin/parity handling,
   source and nested order, optional/error forms, declaration/definition
   splits, forward references, duplicate/conflict behavior, compatibility
   parity and exact downstream phase-B acceptance, rather than only opcode
   names; `LirModule::target_profile` and rendered `data_layout` have no
   semantic Raw destination and cannot influence target-independent Raw facts;
4. distinguish target architecture from checked-in implementation truth.
   Unsupported diagnostics count only as fail-closed evidence. A valid current
   row remains incomplete until its typed destination, wiring and Raw verifier
   are present and lossless;
5. state that current LIR is complete and immutable for this route. Correct
   stale `source gap` language by mapping present facts to the receiving side
   and by removing imagined non-current producer forms from phase-A acceptance;
6. require closed dispatch maintenance tripwires for the variant counts and a
   maintained field-family checklist. Adding a future LIR alternative cannot
   fall through silently, but the docs child itself does not implement that
   machinery;
7. require module-transactional failure: validate before mutation where
   possible, poison/destroy unpublished state after mutation failure, preserve
   stable source-located diagnostics and structured causes, and publish only
   the exact fully verified draft revision;
8. preserve Raw BIR as target-independent and unallocated. It may retain opaque
   target-authored asm bytes and existing requirement tokens, but may not
   interpret ABI placement, constraints, register classes, homes, frame state,
   target profiles/layout text, target opcodes, relocations, MIR, emission, or
   assembler syntax. Genuinely target-independent typed LIR facts remain
   auditable through their own rows without parsing `data_layout`; C1 later
   selects its own exact `TargetProfile`, and C2 owns target-layout derivation;
9. define phase-B input as a move-only verified `RawBir` whose every current
   LIR fact has one typed owner, deterministic iteration, exact def-use/CFG and
   no compatibility string acting as missing identity. Phase B accepts no
   importer maps, partial draft, unsupported valid row, or hidden side table.

### Deferred idea-734 prerequisite and handoff boundary

`ideas/open/734_lir_to_new_bir_container_completeness.md` remains **open,
deferred, and inactive**. It is not Child A and is not one of the six docs
children. This packet neither edits nor activates it.

Child A is idea 734's strict documentation prerequisite. Only after Child A's
matrix is accepted may a later lifecycle decision use idea 734 to implement
the checked-in new-BIR containers, importer dispatch/wiring, builders/views,
full Raw verifier, build inclusion, and focused/broader tests required by those
accepted rows. Idea 734 may not reinterpret Child A as authority to change LIR,
invent new producer families, canonicalize, perform target preparation or ABI
work, allocate, build MIR, emit, parse assembly, revive legacy BIR, or claim
completion through unsupported diagnostics. The inline-asm carrier exception
enters idea 734 only if new evidence overturns the finding above; current
evidence forbids that exception.

## Suggested Next

- Execute plan Step 3: create exactly six collision-safe, documentation-only
  open child ideas A through F. Child A must copy the matrix contract and idea-
  734 boundary above; B through F consume their immediate predecessor's
  accepted output contract. Keep idea 734 deferred/inactive and idea 733 in
  draft.

## Watchouts

- The phase-A matrix is an intake/documentation contract, not authorization to
  implement its target destinations or to accept current design prose as
  implementation truth.
- Do not reintroduce README “source gap” wording that redirects current facts
  into LIR. Missing desired forms outside the current 38/6 surface are not
  phase-A inputs; missing receipt of current facts belongs to new BIR and/or the
  importer.
- Preserve the inline-asm finding: ordinary positions, roles, constraint
  indices and original text already suffice on the LIR side. The current gap is
  the Raw-BIR role/index receiving owner and wiring.

## Proof

- Docs-only packet: no canonical regression log applies and neither
  `test_before.log` nor `test_after.log` was changed.
- The following read-only mechanical proof extracts both variants directly
  from `src/codegen/lir/ir.hpp`, compares them to individually named rows above
  (no catch-all), and checks the complete required metadata-family key set:

```bash
python3 - <<'PY'
from pathlib import Path
import re

source = Path('src/codegen/lir/ir.hpp').read_text()
todo = Path('todo.md').read_text()

def source_variants(alias):
    match = re.search(rf'using {alias} = std::variant<(.*?)>;', source, re.S)
    assert match, alias
    return re.findall(r'\bLir[A-Za-z0-9_]+\b', match.group(1))

def recorded_variants(alias, section_end):
    section = todo.split(
        f'### Exhaustive `{alias}` source/receiving matrix', 1)[1].split(
        section_end, 1)[0]
    return re.findall(rf'^\| `{alias}::(Lir[A-Za-z0-9_]+)` \|', section, re.M)

inst_source = source_variants('LirInst')
term_source = source_variants('LirTerminator')
inst_rows = recorded_variants(
    'LirInst', '### Exhaustive `LirTerminator` source/receiving matrix')
term_rows = recorded_variants(
    'LirTerminator', '### Required metadata-family source/receiving checklist')
assert len(inst_source) == 38 and len(inst_rows) == 38
assert len(term_source) == 6 and len(term_rows) == 6
assert inst_rows == inst_source, (inst_source, inst_rows)
assert term_rows == term_source, (term_source, term_rows)
assert len(set(inst_rows)) == len(inst_rows)
assert len(set(term_rows)) == len(term_rows)

expected_families = {
    'module-context', 'stable-identities', 'operand-kinds', 'type-system',
    'functions-signatures', 'blocks-cfg-order', 'values-def-use',
    'stack-objects-allocas', 'globals-objects', 'initializers', 'strings',
    'externs', 'specializations', 'intrinsic-requirements',
    'inline-asm-metadata', 'producer-indexes-caches', 'source-order-origin',
    'module-publication',
}
family_section = todo.split(
    '### Required metadata-family source/receiving checklist', 1)[1].split(
    '### Inline-assembly carrier finding', 1)[0]
family_rows = re.findall(r'^\| `([a-z0-9-]+)` \|', family_section, re.M)
assert len(family_rows) == len(set(family_rows))
assert set(family_rows) == expected_families, (
    sorted(expected_families - set(family_rows)),
    sorted(set(family_rows) - expected_families),
)
print(f'PASS LirInst={len(inst_rows)} LirTerminator={len(term_rows)} '
      f'metadata_families={len(family_rows)}')
PY
git diff --check
```

- Result:

```text
PASS LirInst=38 LirTerminator=6 metadata_families=18
git diff --check: PASS
```
