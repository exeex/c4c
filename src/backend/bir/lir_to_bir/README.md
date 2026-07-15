# LIR-to-Raw-BIR Import Contract

Contract-Status: checked implementation ledger
Implementation-Status: partial
Kind: stage
Plan-Step: 1
Upstream: complete current typed `codegen::lir::LirModule`
Downstream: private `ModuleDraft` submitted to the A2 Draft/Raw publication gate
Owner-Path: `src/backend/bir/lir_to_bir/README.md`
Last-Reconciled-Commit: none

## Purpose

This document owns the A1 contract for lossless, deterministic translation of
the complete current typed LIR surface into one private new-BIR module draft.
Current LIR is authoritative, complete, correct, and immutable for this route.
A missing receiving type or importer case is a new-BIR and/or importer gap;
calling valid input unsupported proves fail-closed behavior, not receipt.

The contract is target-independent. `LirModule::target_profile` and rendered
`data_layout` are validation/origin/parity context only and have no semantic
Raw destination. C1 later selects its own exact `TargetProfile`; C2 derives
layout facts. A1 never parses layout text to create Raw semantics.

## Owns

- closed dispatch for exactly 38 `LirInst` alternatives, six
  `LirTerminator` alternatives, and the 18 metadata families below;
- source-to-draft identity maps while the transaction is private, deterministic
  source/nested order, forward-reference resolution, and duplicate/conflict
  detection;
- classification of each LIR field as semantic authority, compatibility
  mirror, producer index/cache, or validation evidence;
- creation of typed draft specifications for the single Raw owner named by
  each row and stable, source-located import failures;
- consuming private construction into exactly one A2 publication attempt.

Old/new LIR twins remain distinct source rows but converge on one semantic BIR
owner. A future LIR alternative requires a new individually named row, closed
visitor handling, and an updated mechanical count before it can be accepted.

## Does Not Own

- LIR schema or producer changes, including any inline-asm carrier exception;
- Raw storage definitions, full Raw verification, or the authority to mint
  `RawBir`; those belong to core and the shared A2 verifier;
- canonicalization, analysis products, target/profile selection, target layout,
  ABI placement, constraint interpretation, allocation, frame state, MIR,
  emission, object/link behavior, or assembler parsing;
- semantic reconstruction from names, pointers, vector position, rendered
  text, caches, hash iteration, or dense analysis indices;
- independent publication or implementation authority for build-excluded
  `lir_to_bir/*.cpp` migration files.

## Inputs

The input is one LIR module that has passed the public LIR structural verifier.
Structured fields and stable IDs are semantic authority where present.
Rendered strings are payload only where a row explicitly says so; otherwise
they are compatibility mirrors that may check parity but cannot create a
missing type, identity, operand, CFG edge, initializer topology, or effect.
Producer maps/caches are validation evidence against ordered semantic vectors,
never a second entity set.

Every optional form is explicit: declaration/definition, direct/indirect call,
void/non-void result, variadic/void parameter list, static/dynamic allocation,
ordinary output/read-write inline asm, default/case CFG edges, and absent
structured mirrors. Missing required facts reject the whole transaction.

## Outputs

A1 has no public partial output. Successful private construction yields one
move-only `ModuleDraft` with typed stable identities, deterministic orders,
exact def-use and terminator-owned CFG, no unresolved reservation/fixup, and no
semantic target or allocation state. A2 alone consumes that exact draft through
`verify_and_publish_raw(ModuleDraft&&)` and may mint one `RawBir`.

No importer map, diagnostic-only report, partial function/global set, stale
analysis, fixup table, builder capability, or compatibility side table crosses
the A1 boundary. Failure destroys unpublished state.

## Adjacent-Stage Contract

Upstream authority is [`ir.hpp`](../../../codegen/lir/ir.hpp) and its public
model subheaders. The subordinate [memory import document](memory/README.md)
may refine memory-row validation, but it is not a second dispatcher or
publication owner. Core receiving ownership is documented by
[Raw BIR core](../core/README.md), and the sole publication boundary is the
[A2 verifier](../verify/README.md).

A2 success hands one move-only, fully verified, target-independent,
unallocated `RawBir` to phase B. The exact B1 consumer clause is
[`passes/legalize/README.md`](../passes/legalize/README.md): its only input is
an immutable published `RawBir` view. B1 receives no draft, importer map,
unsupported valid row, hidden side table, target context, or partial
capability. The phase-B contract is historical evidence from closed
[Child B](../../../../ideas/closed/736_bir_phase_b_canonical_document_convergence.md).
Open [umbrella 732](../../../../ideas/open/732_bir_stage_document_convergence_umbrella.md)
actively owns the ordered A-F documentation convergence; its B-F planning is
converged. Only Step 8B awaits a named completed revision of the parallel
importer route plus inspectable code and matching test evidence.

Historical idea 734 (`lir_to_new_bir_container_completeness`) is evidence, not
current lifecycle authority. This ledger does not itself claim that a missing
container, importer case, verifier rule, or proof has landed.

## Exhaustive Instruction Input Matrix

Authority classes are `S` semantic, `C` compatibility mirror/payload as named,
and `V` validation evidence. “Raw verifier” means the full shared A2 gate.
Every proof cell requires both the positive case and malformed plus neighboring
coverage; it is an obligation, not a claim that missing implementation exists.

| Alternative | Exact source authority and class | Typed Raw destination and importer rule | Current disposition | Raw verifier rule and stable failure | Positive + malformed/neighbor proof |
|---|---|---|---|---|---|
| `LirInst::LirConstInt` | S: `result`, `TypeSpec type`, `long long value` | typed `ConstantId`; bind source result once, emit no fabricated runtime op | container+wiring missing | exact type/value/result; reject unrepresentable value, no truncation/publication | + boundary values; - invalid type/duplicate result; N float constant |
| `LirInst::LirConstFloat` | S: `result`, `TypeSpec type`, `double value` | typed floating `ConstantId`; bind result once | container+wiring missing | exact value bits/type/result; reject loss or text reconstruction | + finite/edge current values; - invalid type/result; N integer constant |
| `LirInst::LirLoad` | S: `result`, `type`, `ptr` IDs | shared typed `Load` payload with ordinary use/result edges | container+wiring missing | pointer/value type, ownership, unique result; reject missing/foreign ID | + valid load; - bad pointer/type; N `LirLoadOp` |
| `LirInst::LirStore` | S: `ptr`, `val` IDs and `type` | shared typed `Store` payload with ordered uses | container+wiring missing | both uses and stored type; reject unresolved/foreign use | + valid store; - mismatch/missing ID; N `LirStoreOp` |
| `LirInst::LirBinary` | S: result/type/lhs/rhs IDs and current integer `op` | shared typed binary/unary payload; closed current-op mapping only | container+wiring missing | admitted opcode, arity, types, uses; reject unknown opcode | + each admitted op; - invalid op/type; N `LirBinOp` |
| `LirInst::LirCast` | S: result, `from_type`, `to_type`, operand | shared typed `Cast`; import only a uniquely classified current cast | container+wiring missing | source/destination/result consistency; reject ambiguity without text inference | + valid cast; - invalid pair/use; N `LirCastOp` |
| `LirInst::LirCmp` | S: result, predicate, lhs/rhs IDs | shared typed `Compare`; preserve current predicate/domain | container+wiring missing | operand compatibility and boolean result; reject invalid predicate | + integer/float-valid forms; - domain mismatch; N `LirCmpOp` |
| `LirInst::LirCall` | S: result, return type, direct name/indirect pointer, ordered arg IDs | typed `Call` with symbol-or-value callee, signature/result and ordered args | container+wiring missing | coherent callee/signature/result/args; reject name invention or partial graph | + direct/indirect/void/value; - incoherence; N `LirCallOp` |
| `LirInst::LirGep` | S: result, base type/pointer, ordered index IDs | shared typed `GetElementPtr`; retain exact ordered path | container+wiring missing | base/result/index types and identities; reject invalid path, no folding | + multi-index path; - bad base/index; N `LirGepOp` |
| `LirInst::LirSelect` | S: result/type, condition and arm IDs | shared typed `Select` | container+wiring missing | boolean condition, equal arm/result types; reject invalid identity | + both arm forms; - condition/type mismatch; N `LirSelectOp` |
| `LirInst::LirIntrinsic` | S: result ID, name, ordered arg IDs | typed target-independent `Intrinsic` registry payload and ordinary edges | container+wiring missing | recognized current semantic entry, arity/type/result; reject unknown input | + known entries; - unknown/arity mismatch; N module requirement flags |
| `LirInst::LirInlineAsm` | S payload: result ID, asm/constraint strings, ordered operand IDs | core `InlineAsm` with ordinary edges and opaque exact strings | container+wiring missing | edge/type/order and byte preservation; reject malformed identity, never parse | + embedded bytes/multiple operands; - bad ID; N `LirInlineAsmOp` |
| `LirInst::LirMemcpyOp` | S: typed dst/src/size operands, volatility | typed semantic `Memcpy`; preserve order/size/volatility | container+wiring missing | operand kinds/types and dynamic size; reject invalid use, no scalarization | + volatile/dynamic forms; - bad kind; N `LirMemsetOp` |
| `LirInst::LirVaStartOp` | S: typed `ap_ptr` | typed `VaStart` semantic payload | container+wiring missing | pointer use/effect; reject malformed use | + valid pointer; - bad kind/type; N `LirVaEndOp` |
| `LirInst::LirVaEndOp` | S: typed `ap_ptr` | typed `VaEnd` semantic payload | container+wiring missing | pointer use/effect; reject malformed use | + valid pointer; - bad kind/type; N `LirVaStartOp` |
| `LirInst::LirVaCopyOp` | S: typed destination/source pointers | typed `VaCopy`; preserve ordered uses | container+wiring missing | pointer types/order/effect; reject invalid input without ABI interpretation | + valid pair; - missing/mistyped use; N `LirVaArgOp` |
| `LirInst::LirStackSaveOp` | S: typed ordinary result operand | typed `StackSave` and ordinary result | container+wiring missing | one pointer-like result and order; reject invalid definition | + valid result; - duplicate/mistyped result; N restore |
| `LirInst::LirStackRestoreOp` | S: typed saved-pointer operand | typed `StackRestore` | container+wiring missing | valid owned use and ordering; reject unresolved value | + matching save/restore; - foreign/missing use; N save |
| `LirInst::LirAbsOp` | S: result/argument operands and integer type ref | typed target-independent `Abs` | container+wiring missing | exact current type and identities; reject malformed input, invent no flags | + current widths; - type/result mismatch; N intrinsic |
| `LirInst::LirIndirectBrOp` | S: address operand and ordered target-label strings | sole typed `IndirectJumpTerm`; reconcile only last-op/default-unreachable carrier rule | container+wiring missing; a second ordinary BIR op is stale | exact order/unique targets/sentinel; reject hidden edge or unresolved label | + multi-target carrier; - nonlast/bad target; N `LirIndirectBr` |
| `LirInst::LirExtractValueOp` | S: result/aggregate operands, aggregate type, index | typed `ExtractValue` | container+wiring missing | shape/index/result/IDs; reject out-of-range or invented text semantics | + valid field; - bad index/type; N insert-value |
| `LirInst::LirInsertValueOp` | S: result/aggregate/element operands, types, index | typed `InsertValue` | container+wiring missing | aggregate/element/index/result consistency; reject mismatch, no leaf expansion | + value/special token; - bad index/type; N extract-value |
| `LirInst::LirLoadOp` | S: result/pointer operands and `LirTypeRef` | same typed `Load`; structured kind/type drives receipt | container+wiring missing | kind/type/result checks; reject semantic `RawText` | + typed load; - raw/unresolved operand; N `LirLoad` |
| `LirInst::LirStoreOp` | S: value/pointer operands and `LirTypeRef` | same typed `Store` | container+wiring missing | use/type checks; reject without decoding rendered fragments | + typed store; - raw/mismatched use; N `LirStore` |
| `LirInst::LirMemsetOp` | S: typed dst/byte/size operands, volatility | typed semantic `Memset` | container+wiring missing | kinds/widths/order/volatility; reject invalid input, no scalarization | + volatile/dynamic forms; - bad byte/size; N memcpy |
| `LirInst::LirCastOp` | S: result/operand, `LirCastKind`, from/to refs | same typed `Cast`; closed handling of every current cast kind | container+wiring missing | exact kind/type/use/result; reject unknown/incoherent form | + every cast kind; - mismatched kind/type; N legacy cast |
| `LirInst::LirGepOp` | S: result/pointer, element type, `inbounds`; C: ordered current index strings | same typed `GetElementPtr`; retain current path only under explicit admitted representation | container+wiring missing | base/result/type/inbounds/path; reject inadmissible index, no folding | + in/out-of-bounds flag forms; - malformed path; N legacy GEP |
| `LirInst::LirCallOp` | S: result/return/callee, direct link ID, optional signature, structured args/type refs/extensions; C: suffix/args strings | same typed `Call`; structured facts win, mirrors parity-only | container+wiring missing | all optional forms coherent; reject missing identity/signature or mirror authority | + direct/indirect/variadic/attrs; - incoherence; N legacy call |
| `LirInst::LirBinOp` | S: result/lhs/rhs, typed opcode ref and type ref | same typed binary/unary payload | container+wiring missing | closed opcode/arity/type/use rules; reject untyped/unknown ref | + all typed opcodes; - unary/binary mismatch; N legacy binary |
| `LirInst::LirCmpOp` | S: result/lhs/rhs, float flag, predicate ref, type ref | same typed `Compare` | container+wiring missing | predicate belongs to domain and operand/result types; reject inference | + integer/float predicates; - domain mismatch; N legacy cmp |
| `LirInst::LirPhiOp` | S current payload: result/type and ordered `(value,label)` pairs | typed `Phi` keyed to exact derived CFG edge identities | container+wiring missing | exactly one typed incoming per edge and multiplicity; reject ambiguity | + loop/parallel-safe case; - missing/extra/ambiguous; N branch/switch |
| `LirInst::LirSelectOp` | S: result/type/condition/arm operands | same typed `Select` | container+wiring missing | condition/arm/result type and identities; reject raw/unresolved use | + typed select; - type mismatch; N legacy select |
| `LirInst::LirInsertElementOp` | S: result/vector/element/index operands and types | typed `InsertElement` | container+wiring missing | lane/index/type/result; reject malformed lane/token | + value/poison forms; - invalid index/type; N extract-element |
| `LirInst::LirExtractElementOp` | S: result/vector/index operands and types | typed `ExtractElement` | container+wiring missing | vector/index/result consistency; reject malformed access | + valid lanes; - bad index/type; N insert-element |
| `LirInst::LirShuffleVectorOp` | S: result, two vectors, mask operands and type refs | typed `ShuffleVector` | container+wiring missing | input/mask/result shapes; reject mismatch, no target selection | + valid mask; - malformed shape; N vector insert/extract |
| `LirInst::LirVaArgOp` | S: result/ap-pointer operands and result type | typed `VaArg` | container+wiring missing | use/result/type; reject invalid form, no ABI expansion | + scalar/aggregate current types; - bad pointer/type; N va-copy |
| `LirInst::LirAllocaOp` | S: result, element type, optional count, alignment | typed semantic local/`Alloca`; preserve static/dynamic form/order | container+wiring missing | type/count/alignment/result; reject invalid form, no frame placement | + static/dynamic/align; - bad count/align; N stack object |
| `LirInst::LirInlineAsmOp` | S: ordinary bindings(value/type/role/index), original texts, clobbers, effects; V: optional `insn_r`; C: rendering mirrors | core `InlineAsm` ordinary edges/opaque payload plus typed role/index attachment; `insn_r` audit-only | generic edges/payload proved; role/index container+wiring missing | order/roles/indices/types/read-write/bytes; reject interpreted `insn_r` or partial receipt | + input/output/read-write and embedded bytes; - bad role/index/pair; N legacy asm |

## Exhaustive Terminator Input Matrix

| Alternative | Exact source authority and class | Typed Raw destination and importer rule | Current disposition | Raw verifier rule and stable failure | Positive + malformed/neighbor proof |
|---|---|---|---|---|---|
| `LirTerminator::LirBr` | S: `target_label` current target carrier | checked-in `JumpTerm(BlockId)`; resolve uniquely | already-proved bounded coverage | one owned target and sole successor; reject missing/duplicate target | + forward/back edge; - unknown label; N conditional branch |
| `LirTerminator::LirCondBr` | S current payload: condition name and ordered true/false labels | checked-in `CondJumpTerm`; resolve ordinary boolean value and both blocks | importer wiring missing; source-absence prose stale | boolean type, targets, equal/parallel edges; reject invalid facts | + distinct/equal targets; - bad condition/target; N jump |
| `LirTerminator::LirRet` | S current payload: optional value and type strings | checked-in `ReturnTerm`; resolve zero/one semantic value | void proved; non-void wiring missing | agrees with function result; reject missing/mistyped value, no ABI lanes | + void/value return; - signature mismatch; N unreachable |
| `LirTerminator::LirSwitch` | S: selector name/type, default, ordered value/label cases | typed `SwitchTerm`; preserve selector, case order and exact targets | container+wiring missing | duplicate/conflict and successor multiplicity; reject invalid case/target | + multi/parallel cases; - duplicate conflict; N conditional branch |
| `LirTerminator::LirIndirectBr` | S: structured address `LirValueId`, ordered target `LirBlockId`s | typed `IndirectJumpTerm`; reconcile with op carrier only by sentinel rule | container+wiring missing; existing structured form is not a source gap | valid owned address/complete ordered targets; reject invalid target/set | + multi-target; - foreign/missing ID; N indirect-br op |
| `LirTerminator::LirUnreachable` | S: empty typed alternative | checked-in `UnreachableTerm` | already-proved bounded coverage | exactly one terminator and no successor/fallthrough | + terminal block; - extra/fallthrough state; N return |

## Exhaustive Metadata-Family Input Matrix

| Family key | Exact source authority and class | Typed Raw destination or non-destination and importer rule | Current disposition | Raw verifier rule and stable failure | Positive + malformed/neighbor proof |
|---|---|---|---|---|---|
| `module-context` | V: `LirModule::target_profile`, rendered `data_layout` | explicit validation/origin/parity-only non-destination; never create Raw facts | stale if called Raw semantics; parity wiring missing | reject any semantic influence or parsed layout; C1 selects profile, C2 derives layout | + ignored/audited context; - attempted semantic use; N target-independent type fact |
| `stable-identities` | S: value/block/stack/global/link/struct IDs and invalid sentinels | typed BIR ID families plus non-authoritative origin mapping | block/asm slice partial; complete container+wiring missing | scope/uniqueness/sentinel/forward-ref rules; reject name replacement | + cross-order valid IDs; - duplicate/foreign/invalid; N source order |
| `operand-kinds` | S: `LirOperand` text+kind for SSA/global/label/immediate/special/raw | closed typed value/constant/symbol/block destination or explicit compatibility/error form | missing outside asm; absent-classification prose stale | every kind imports or fails; `RawText` cannot create identity | + each kind; - misclassified/raw semantic use; N type system |
| `type-system` | S: `TypeSpec`, `LirTypeRef` kind/width/name ID, opcode/predicate refs, cast kind, struct declarations; C: rendering mirrors | interned type graph, typed discriminants and named aggregate definitions | bounded scalar/pointer partial; complete container+wiring missing | interning/kind/width/recursion/parity; reject invented fields | + recursive/width/opcode forms; - conflict/raw authority; N globals/functions |
| `functions-signatures` | S: function name/link ID, flags, return/params, variadic/void-list, structured refs; C: `signature_text` | typed function/signature/symbol/ordered parameter values; text parity-only | zero-param void shell partial; remainder container+wiring missing | all optionals and declaration/body coherence; reject text-derived semantics | + decl/def/params/results/variadic; - conflict/body-on-decl; N externs |
| `blocks-cfg-order` | S: block ID/label/ordered insts/terminator, block vector and explicit entry | typed blocks/order/terminator/entry; label is debug/origin only | shell partial; explicit nonfirst entry/full CFG wiring missing | entry by ID, exact order/target ownership; reject positional policy | + nonfirst-ID/order/edges; - duplicate/bad entry; N values |
| `values-def-use` | S: result/use IDs, parameters, phi incoming and asm bindings | ordinary typed parameter/constant/instruction-result definitions and exact uses | bounded asm partial; general container+wiring missing | predeclare, uniqueness/type/owner/forward uses; reject hidden value system | + forward/loop uses; - duplicate/foreign/missing; N CFG order |
| `stack-objects-allocas` | S: stack object ID/name/type/align/VLA and ordered inline/hoisted allocas | typed semantic local objects and `Alloca` operations in source order | container+wiring missing | identity/type/alignment/count/order; reject dropping/frame placement | + static/dynamic/hoisted; - bad ID/count/order; N memory ops |
| `globals-objects` | S: global/link IDs, type/internal/const/alignment/extern flags; C: linkage/qualifier/type mirrors | typed symbol/global/object/declaration record | container+wiring missing; imagined absent metadata is not input | preserve current flags/identity/order, parity mirrors; reject conflict/invention | + decl/def/internal/const; - duplicate/conflict; N initializer |
| `initializers` | S: ordered initializer function link IDs; C current payload: `init_text` | typed initializer owner with explicit compatibility-payload classification; never parse topology | container+wiring missing; future-tree prerequisite wording stale for intake | referenced IDs and exact lossless disposition; reject parsed/invented topology | + zero/symbol current forms; - dangling ID/unreceivable payload; N global/string |
| `strings` | S current payload: ordered pool name/raw bytes/byte length; V: map/counter parity | typed string/data object with exact bytes/spelling classification, length and symbol | container+wiring missing; absent-row prose stale | content/length/name/cache parity and vector order; reject hash-order identity | + empty/embedded/ordered strings; - length/cache conflict; N globals |
| `externs` | S: ordered extern declarations/info with return type/attr/link ID; V: link/name maps | typed external symbol/declaration/signature; maps parity-only | container+wiring missing; map-as-authority stale | coherent merge by stable ID, map parity; reject declaration conflict | + repeated coherent decl; - type/link conflict; N function decl |
| `specializations` | S: ordered key/origin/mangled name/link ID | typed specialization/origin record linked to ordinary symbol | container+wiring missing | order/identities/parity; reject dangling symbol or mismatch | + multiple ordered entries; - bad link/mangle parity; N symbols |
| `intrinsic-requirements` | V: `need_va_start`, `need_va_end`, `need_va_copy`, `need_memcpy`, `need_memset`, `need_stacksave`, `need_stackrestore`, `need_abs`, `need_ptrmask`, `prefer_semantic_va_ops` | validation-only typed requirement/parity result; never synthesize operations | importer parity wiring missing; second instruction authority stale | cross-check actual ops/declarations; reject inconsistent flag set | + matching flags; - missing/spurious flag; N intrinsic rows |
| `inline-asm-metadata` | S: bindings/roles/indices/original texts/clobbers/effects; V: `insn_r`; C: render mirrors | ordinary value edges, opaque payload and typed role/index attachment; audit-only `insn_r` | payload/edges proved; role/index container+wiring missing | bytes/order/roles/indices/types/read-write; reject parsing/allocation | + `r`,`=r`,`f`,`VR` evidence by position; - bad pair/index; N asm variants |
| `producer-indexes-caches` | V: extern/struct/string/intern/name/layout indexes, observations and counters | validation/import report only unless an ordered semantic row names a destination | semantic-input claims stale; parity wiring missing | deterministic comparison to ordered authority; reject pointer/hash/storage identity | + coherent cache; - mismatch/hash-order dependence; N source order |
| `source-order-origin` | S: all module/function/block/inst and nested argument/case/index/binding orders; C/V: labels/sites | typed deterministic module/function/block orders plus non-authoritative origin/debug | function/block/inst partial; remaining container+wiring missing | preserve every nested order and stable diagnostic site; reject hash leakage | + reordered declarations/nested lists; - nondeterministic/missing site; N identities |
| `module-publication` | S/V: complete verified LIR module and every row above | one private `ModuleDraft` consumed once by full A2 Raw gate | bounded foundation adapter proved; full draft/gate path missing | exact revision, no active editor/reservation/fixup; any failure publishes nothing | + complete module publication; - one fault per family and rollback; N B1 handoff |

## Ordered Behavior

1. Run LIR verification and inventory all 38/6/18 rows before allocating BIR
   IDs. Reject unknown/new alternatives until their explicit row lands.
2. Validate caches/indexes against ordered authorities. They never determine
   identity or iteration order.
3. Predeclare typed identities in source order: types, symbols/objects,
   functions, blocks, parameters/results, and storage objects. Preserve the
   explicit entry ID and declaration/definition splits.
4. Reserve definitions/results in source order, define non-phi instructions,
   resolve terminators and exact successor slots, then resolve phis/forward
   uses. No canonicalization, folding, scalarization, ABI expansion, or target
   selection occurs.
5. Finish only when every reservation, fixup, definition, terminator, and
   metadata disposition is resolved. Consume the private draft once at A2.

Duplicate old/new operation shapes converge before publication. A Raw module
cannot contain compatibility alternatives, unresolved reservations, importer
sentinels, or a generic “unsupported” instruction placeholder.

## Checked Source And Build Census

- Typed source authority is `codegen/lir/ir.hpp`, `types.hpp`, and
  `operands.hpp`: the checked aliases contain exactly the 38 instruction rows
  and six terminator rows above. The metadata matrix covers every current
  module/function/block/object field, including ordered vectors, stable IDs,
  invalid sentinels, link/struct/type-tag storage and indexes, allocation
  counters, intrinsic flags, layout observations, and string-pool caches.
- Checked-in Raw storage has only module/function/block/instruction/value IDs,
  scalar/pointer `Type`, function signature/parameters, ordinary SSA edges,
  `InlineAsmNode`, and the four terminators `JumpTerm`, `CondJumpTerm`,
  `ReturnTerm`, and `UnreachableTerm`. Its builder and immutable views expose
  only those families.
- `FoundationVerifier` is reachable from `ModuleBuilder::publish()` and checks
  storage/order, signature parameters, ordinary definitions/uses, the closed
  inline-asm payload, the four checked-in terminators, and link-name indexing.
  A matrix row that needs another type, object, instruction, terminator, entry,
  metadata, or publication rule therefore has a real verifier gap even when
  the table can name its required rule.
- Production calls from `backend.cpp` reach only top-level
  `bir/lir_to_bir.cpp`. `src/backend/CMakeLists.txt` deliberately excludes
  every nested `bir/lir_to_bir/*.cpp`, so those files are legacy behavior
  evidence, never current implementation coverage.
- `LEGACY_COVERAGE.md` was checked only as a historical family checklist. It
  adds no typed destination and cannot override the current LIR aliases, Raw
  storage, build graph, importer, or verifier.

## Bounded Implementation Packet Order

Each packet must change storage/IDs, builder, immutable view, verifier,
production importer, and neighboring positive/negative proof together. A
later packet may not use an earlier packet's missing destination as a text or
position fallback.

1. **Step 2A — closed typed LIR type receipt.** Extend the Raw `Type`
   representation and verifier to losslessly receive current structured
   `TypeSpec` and every `LirTypeKind` alternative with its typed width/name
   identity needed by signatures and ordinary values; add one importer
   conversion shared by function signatures and inline-asm bindings.
   Explicitly reject semantic `RawText`, missing integer/VRM widths, invalid
   struct IDs, and conflicting named-type definitions. Prove every admitted
   kind plus malformed and neighboring kinds. Do not add globals, objects,
   constants, new opcodes, or new terminators in this packet.
2. **Step 2B — module type/name tables and stable module IDs.** Add the typed
   interned/named aggregate definitions, deterministic orders, builders/views,
   and cache-parity verification required by `struct_decls`, `link_names`, and
   `struct_names`; keep compatibility type text and storage pointers
   non-authoritative.
3. **Step 2C — foundational constant/value receipt.** Add typed constants and
   complete parameter/result reservation and forward-use identity without an
   instruction-shaped placeholder.
4. **Step 3A — symbols, externs, globals, and strings.** Add stable symbol and
   object IDs, exact ordered declarations/definitions, external signatures,
   and byte-exact string data with index/cache parity.
5. **Step 3B — initializers and specializations.** Add typed initializer
   ownership/current compatibility-payload disposition, symbol references,
   and ordered specialization records.
6. **Step 4A — complete signatures and explicit entry/CFG identities.** Remove
   the zero-parameter/void/first-block restrictions only with entry storage,
   parameter/result views, and verifier coverage.
7. **Step 4B — stack objects and alloca ordering.** Add local object IDs,
   hoisted/inline alloca semantics, alignment/count checks, and source order
   without frame placement.
8. **Step 5 — ordinary instruction families.** Land constants, scalar
   arithmetic/compare/cast/select, aggregate/vector, memory/stack/variadic,
   calls/intrinsics, phi, and inline-asm role/index attachment as separately
   proven coherent family slices in that dependency order.
9. **Step 6 — remaining terminators.** Wire checked-in conditional and
   non-void return receipt, then add switch and indirect-jump destinations and
   reconcile the legacy indirect-branch instruction carrier by its sentinel
   rule.
10. **Steps 7-8 — dispatcher/build/publication closure.** Remove bounded
    rejections only after their rows are typed and verified, integrate each
    production translation unit once, and prove exhaustive dispatch plus
    whole-module rollback and lossless publication.

The first coherent C++ packet is therefore **Step 2A only**. It is bounded by
the current `TypeSpec`/`LirTypeRef` discriminant and identity facts and supplies
the shared type conversion required by every later signature, object, value,
instruction, and terminator packet.

## Invariants

- Current LIR is complete and immutable for this route. Every valid current
  fact has one explicit import-or-fail disposition; unsupported receipt is not
  successful coverage.
- Stable semantic identity comes only from typed IDs. Names, pointers, vector
  positions, rendered strings, caches, hash order, and dense indices cannot
  create or replace identity.
- Source and every nested order are deterministic. Terminators alone own CFG
  successors, and every ordinary value uses the common def-use model.
- Raw remains target-independent and unallocated. Validation/origin/parity
  context, private importer maps, fixups, and partial capabilities never become
  published semantics.
- Every failure is module-transactional and publishes no draft, function
  subset, cache entry, or stage capability.

The current `LirInlineAsmOp` carrier is sufficient. Ordinary `LirOperand`
values, exact `LirTypeRef`s, `Input`/`Output`/`ReadWrite` roles,
`constraint_index`, original constraint text, ordered clobbers, side effects,
and byte-preservable `original_asm_text` already record reviewed requirements
such as `r`, `=r`, `f`, and `VR` against ordinary positions and roles.

No LIR edit or exception is justified. The remaining A1/A2 gap is typed Raw
ownership and importer receipt of role/index facts. Raw does not gain an
`InlineAsmOperand`, name-binding table, special value-ID system, parsed
alternative/tie/group authority, allocator, projection, target opcode, or asm
parser. Optional `insn_r` is audit evidence only. The later assembler alone
interprets opaque asm bytes.

## Verification and Publication

The shared A2 verifier owns all Raw acceptance. It checks type/ID ownership,
def-use, instruction descriptors, terminator-only successor authority,
deterministic entity order, declarations/definitions, globals/initializers,
optional/error forms, absence of target/allocation state, and exact revision.

The target contract is:

```text
verified LirModule
  -> private deterministic import transaction
  -> complete move-only ModuleDraft
  -> verify_and_publish_raw(ModuleDraft&&)
  -> one move-only verified RawBir, or no published state
```

Diagnostic-only validation cannot mint or bless a stage token. The current
checked-in `ModuleBuilder::publish()` foundation adapter is not evidence that
the full `ModuleDraft -> A2` contract is implemented.

## Failure and Diagnostics

Validation accumulates independent source-located errors only while it is safe
to do so. Once mutation starts, the first poisoning error destroys the complete
unpublished transaction. Stable failures distinguish malformed input, missing
receiving container, missing importer wiring, duplicate/conflicting identity,
unresolved value/block/symbol/type, builder failure, and A2 publication failure.

Diagnostics order by import phase, module ordinal, function, block,
instruction, and field. They retain the shared verifier/builder cause without
flattening it. No exception path returns a partial module, successful function
subset, draft, map, capability, or cached proof.

## Analysis and Invalidation

A1 publishes no analysis. Predecessors, dominance, liveness, provenance,
memory effects, call graphs, and publication routes are derived later from a
published exact revision. Private importer maps die with the transaction and
cannot be analysis cache keys. Phase B starts with no inherited mutable A1
analysis state.

## Target and ABI Rules

Raw is target-independent and unallocated. A1 may preserve opaque
target-authored asm bytes and current requirement tokens without interpreting
them. It cannot import `target_profile` or `data_layout` as semantic Raw state,
choose a target, derive layout, classify ABI arguments/results, select register
classes/opcodes/relocations, assign homes/stack offsets, or prepare MIR.

Genuinely target-independent typed LIR facts remain covered by their own rows;
their receipt never requires parsing rendered target/layout text.

## Implementation State

The build includes only top-level
[`lir_to_bir.cpp`](../lir_to_bir.cpp). [`CMakeLists.txt`](../../CMakeLists.txt)
explicitly excludes every nested `bir/lir_to_bir/*.cpp`; those files and their
designs do not prove runtime coverage.

The checked-in importer currently proves only a bounded bootstrap:

- it rejects globals, strings, externs, type declarations/layout observations,
  intrinsic requirement flags, specialization entries, parameters, variadic
  and non-void functions, stack objects, and hoisted allocas;
- it admits only `LirInlineAsmOp` ordinary instructions, with generic ordinary
  input/result edges, original opaque text, clobbers, and side effects;
- it admits `LirBr`, void `LirRet`, and `LirUnreachable`; it rejects conditional,
  switch, indirect and non-void return receipt;
- checked-in core has only `Opcode::InlineAsm`, a small scalar/pointer `Type`
  set, `JumpTerm`, `CondJumpTerm`, `ReturnTerm`, and `UnreachableTerm`;
- it publishes through the foundation builder adapter, not the complete
  private `ModuleDraft -> full A2 Raw gate` route.

Therefore `Implementation-Status: partial` is exact. Unsupported diagnostics
are useful stable failures but are not positive receiving coverage. Design
tables above name required owners and rules; they do not claim those containers
or wiring are checked in.

## Proof Requirements

- mechanically extract `LirInst` and `LirTerminator` alternatives from current
  `ir.hpp` and require document row order/set equality at exactly 38 and 6;
- require the exact 18 metadata keys and no duplicated/generic inventory row;
- require the metadata spine and core-first headings in exact order;
- resolve every relative Markdown link;
- inspect build inclusion, top-level importer dispatch, current core schema and
  shared A2/B1 adjacency without treating design prose as implementation;
- require neighboring positive and malformed coverage for each row when its
  corresponding implementation packet executes the contract;
- reject completion if any valid current row remains unsupported, loses a fact,
  bypasses the full A2 gate, or lets target/allocation state enter Raw.

## Open Questions

No Step 1 ambiguity blocks Step 2A, and no source-side LIR change is justified.
If a later implementation packet proves that a current typed fact cannot be
received by the named destination without changing LIR, that is a new
evidence-gated route decision rather than permission to widen that packet.

## Review Checklist

- [x] Metadata spine and core-first ownership/input/output/adjacency order are exact.
- [x] Mechanical inventory is exactly 38/6/18 in source order with no omitted row.
- [x] Every row states authority, destination/non-destination, import rule,
      disposition, A2 rule, stable failure, and positive plus malformed/neighbor proof.
- [x] LIR remains complete and immutable; no source-gap wording redirects receipt.
- [x] Inline asm keeps ordinary values and opaque bytes; role/index receipt is the gap.
- [x] Target/profile/layout, ABI, allocation, MIR and emission remain outside Raw.
- [x] Build-included bootstrap truth is distinct from build-excluded design.
- [x] The contract gives one private draft to one full A2 gate; failure publishes nothing.
- [x] B1 receives only the exact move-only verified `RawBir`.
- [x] Historical idea 734 is evidence only; active 732 Step 8B waits for a named completed importer revision and matching code/test evidence.
