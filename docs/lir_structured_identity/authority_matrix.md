# LIR Structured Identity Authority Matrix

Final plan-Step-6 audit checked against `src/codegen/lir/ir.hpp` at HEAD
`e7a24c93d`. The current inventory remains exactly 38 `LirInst` alternatives
and 6 `LirTerminator` alternatives. The final tables below are authoritative
for current implementation status. The detailed Step 2 tables later in this
file are retained as an explicitly historical pre-implementation baseline.

## Final owned authority audit

`LirOperand` now has one closed authority variant: `monostate`, `LirValueId`,
`LinkNameId`, or `LirIntegerImmediate`. Its text and kind remain presentation
and role metadata. `LirGepIndex` is a typed-or-raw carrier, and `LirRet` keeps
the compatibility field spellings `value_str` and `type_str` while their actual
types are `optional<LirOperand>` and `LirTypeRef`; there are no parallel raw
semantic mirrors.

| Owned row | Exact current authority | Actual producer seam | Reachable verifier obligation | Focused proof | Display status | Exact idea-734 receipt row now unblocked |
|---|---|---|---|---|---|---|
| CC-STORE-1: `LirStoreOp.val`, `ptr`, `type_str` | `val = LirIntegerImmediate`; `ptr = LinkNameId`; `type_str = LirTypeRef` | `emit_rval_operand` creates the native literal; `emit_lval_operand` captures the selected `GlobalVar`; `emit_set_assign_value` preserves native authority only across representation-preserving coercion | `verify_global_store_authority` requires native integer payload/range and resolves exactly one module-global owner; generic operand-kind parity also applies | `frontend_lir_call_type_ref` covers ordinary/neighboring stores, misleading display, coercion, and malformed alternatives; focused CLI case is `lir_identity_global_store.c` | presentation-only; the verifier never parses `7` or `@name` | direct selected-global scalar store with native integer value; new-BIR still needs its typed `Store` payload, operand/global mapping, builder, verifier, and importer dispatch |
| CC-LOAD-1: `LirLoadOp.result`, `ptr`, `type_str` | `result = LirValueId`; `ptr = LinkNameId`; `type_str = LirTypeRef` | the global `DeclRef` branch of `emit_rval_operand` selects the global, calls `fresh_value(ctx)`, builds `LirLoadOp`, and returns the same result operand | `verify_global_load_authority` requires direct-global result authority and unique global ownership; `verify_function_value_ownership` rejects invalid/duplicate result IDs | `frontend_lir_call_type_ref` covers ordinary/neighboring loads, misleading display, and malformed ownership; focused CLI case is `global_load.c` | presentation-only; neither `%tN` nor `@name` selects identity | direct selected-global scalar load with one authoritative result; new-BIR still needs typed `Load`, source-ID-to-BIR-value registration, global mapping, builder/verifier, and dispatch |
| CC-GEP-1: `LirGepOp.result`, `ptr`, `indices`, `element_type`, `inbounds` | result `LirValueId`; base `LinkNameId`; every focused index is `LirTypeRef` plus integer immediate (or current-function SSA in admitted authoritative tests) | the global-array `DeclRef` branch calls `fresh_value`, retains the selected global ID, and constructs two typed native i64-zero indices | `verify_authoritative_gep` requires the complete all-typed shape, unique global owner, integer index types, immediate range or current-function SSA, and owned result | `frontend_lir_call_type_ref` covers ordinary neighboring arrays, SSA index, misleading display, and malformed shapes; backend interface covers authority-first adaptation; focused CLI case is `lir_identity_global_array_address.c` | presentation-only for authoritative form; raw `LirGepIndex` is a separate compatibility form | selected-global array-decay GEP with an all-typed ordered path; new-BIR still needs typed `GetElementPtr`, result/base/index mapping, builder/verifier, and dispatch |
| CC-RET-1: `LirRet.value_str`, `type_str` | scalar integer return is optional `LirOperand` carrying `LirIntegerImmediate` or current-function `LirValueId`; type is `LirTypeRef`; void has no value | `stmt.cpp` calls `emit_rval_operand`, preserves native authority only on the no-instruction same-representation path, emits structured synthesized zero, and emits valueless void after any void-expression side effect | `verify_terminator` enforces type parity, void/value shape, integer range, and allowed alternatives; `verify_function_value_ownership` resolves SSA returns in the owning function | `frontend_lir_call_type_ref` covers literal/load/synthesized/void/coercion/misleading/malformed returns; backend interface proves the unchanged new-BIR rejection; focused CLI case is `aarch64_return_zero_smoke.c` | presentation-only for authoritative scalar form; unowned raw non-void compatibility remains accepted | scalar integer `ReturnTerm` value receipt (immediate materialization or source SSA lookup); new-BIR already has optional `ReturnTerm.value` but still needs wiring, function-result parity, value materialization/lookup, verifier coverage, and transactional tests |

All four authoritative producer shapes use native facts before rendering. No
owned verifier, printer, or authority-first adapter reconstructs identity from
display. LLVM output is only a capability observation; the focused C++ tests
inspect native payloads and IDs directly.

The receiver-owned prerequisites and exact resume sequence are recorded in
[`handoff_to_734.md`](handoff_to_734.md).

## Exhaustive current variant comparison

Every alternative below is named individually; there is no catch-all row.
“Raw compatibility” means the generic `LirOperand` carrier exists but the
ordinary producer for that row still supplies `monostate`, or the row retains
an explicit raw string carrier. Those rows are outside idea 741 and are not
silently claimed as idea-734-ready.

| Current alternative | Current identity disposition |
|---|---|
| `LirConstInt` | Native `LirValueId`, `TypeSpec`, and integer payload; producerless legacy contrast, with bounded new-BIR constant receipt already present |
| `LirConstFloat` | Native `LirValueId`, `TypeSpec`, and floating payload; producerless legacy contrast, with bounded receipt already present |
| `LirLoad` | Native legacy result/pointer IDs; producerless and outside 741 |
| `LirStore` | Native legacy pointer/value IDs; producerless and outside 741 |
| `LirBinary` | Native legacy value IDs plus typed fields; producerless and outside 741 |
| `LirCast` | Native legacy value IDs plus typed fields; producerless and outside 741 |
| `LirCmp` | Native legacy value IDs plus predicate; producerless and outside 741 |
| `LirCall` | Native legacy value IDs, but raw direct name remains; producerless and outside 741 |
| `LirGep` | Native legacy result/base/index IDs; producerless and outside 741 |
| `LirSelect` | Native legacy value IDs; producerless and outside 741 |
| `LirIntrinsic` | Native legacy value IDs, but raw selector name remains; producerless and outside 741 |
| `LirInlineAsm` | Native legacy value IDs with opaque text payload; producerless and outside 741 |
| `LirMemcpyOp` | Raw-compatibility operands; semantic flags are native; outside 741 |
| `LirVaStartOp` | Raw-compatibility pointer operand; outside 741 |
| `LirVaEndOp` | Raw-compatibility pointer operand; outside 741 |
| `LirVaCopyOp` | Raw-compatibility pointer operands; outside 741 |
| `LirStackSaveOp` | Authority-capable result carrier, but ordinary producer remains raw compatibility; outside 741 |
| `LirStackRestoreOp` | Raw-compatibility pointer operand; outside 741 |
| `LirAbsOp` | Raw-compatibility value operands with structured type; outside 741 |
| `LirIndirectBrOp` | Raw-compatibility address and raw target labels; outside 741 |
| `LirExtractValueOp` | Raw-compatibility value operands with structured type/index; outside 741 |
| `LirInsertValueOp` | Raw-compatibility value operands with structured types/index; outside 741 |
| `LirLoadOp` | Hybrid: CC-LOAD-1 direct-global scalar route is authoritative; other pointer/result producer shapes remain raw compatibility |
| `LirStoreOp` | Hybrid: CC-STORE-1 direct-global integer route is authoritative; other pointer/value producer shapes remain raw compatibility |
| `LirMemsetOp` | Raw-compatibility operands with native volatility; outside 741 |
| `LirCastOp` | Raw-compatibility operands with typed cast/type facts; outside 741 |
| `LirGepOp` | Hybrid: CC-GEP-1 selected-global array decay is complete authoritative form; other GEP producers remain raw compatibility |
| `LirCallOp` | Direct-call `LinkNameId` and typed signature facts exist, but ordinary result/indirect/argument value identities remain raw compatibility; outside 741 |
| `LirBinOp` | Raw-compatibility operands with typed opcode/type; outside 741 |
| `LirCmpOp` | Raw-compatibility operands with typed predicate/type; outside 741 |
| `LirPhiOp` | Raw-compatibility result and raw incoming value/label pairs; outside 741 |
| `LirSelectOp` | Raw-compatibility operands with typed result type; outside 741 |
| `LirInsertElementOp` | Raw-compatibility operands with typed vector/element types; outside 741 |
| `LirExtractElementOp` | Raw-compatibility operands with typed vector/index types; outside 741 |
| `LirShuffleVectorOp` | Raw-compatibility operands with typed vector/mask types; outside 741 |
| `LirVaArgOp` | Raw-compatibility result/pointer with typed result type; outside 741 |
| `LirAllocaOp` | Raw-compatibility result/count with typed element/alignment facts; outside 741 |
| `LirInlineAsmOp` | Structured binding/type/role containers and bounded new-BIR receipt exist, but ordinary binding value identities remain raw compatibility; outside 741 |
| `LirBr` | Raw target label; existing bounded importer resolves it to `BlockId`; outside 741 |
| `LirCondBr` | Raw condition and target labels; outside 741 |
| `LirRet` | Hybrid: CC-RET-1 authoritative scalar integer or valueless void; other non-void forms remain raw compatibility |
| `LirSwitch` | Raw selector/type/labels with native case values; outside 741 |
| `LirIndirectBr` | Native `LirValueId` address and ordered `LirBlockId` targets; producerless legacy contrast, outside 741 |
| `LirUnreachable` | Closed fieldless variant; bounded new-BIR receipt already present |

Mechanical comparison of these names with the two variants in `ir.hpp`
produces 38/38 instruction matches and 6/6 terminator matches. The lower
tables preserve the more detailed field-level Step 2 evidence but do not
override this final status.

## Historical Step 2 classification and evidence keys

The remainder of this document records the pre-implementation snapshot checked
at HEAD `cbfb6095f`. Present-tense wording inside those historical tables is a
property of that snapshot, not current implementation status.

Authority classes in the historical tables are literal descriptions of the
Step 2 snapshot carrier:

- `LirValueId`, `LirBlockId`, and `LinkNameId` are stable identities.
- `native/typed immediate` includes C++ numeric/enumerated fields, `TypeSpec`,
  `LirTypeRef`, and typed opcode/predicate wrappers.
- `structured-but-text-only LirOperand` stores text plus a classified kind. It
  does **not** own a value, symbol, block, or immediate identity.
- `raw text` is an untyped string or a compound string fragment.

Producer keys name representative actual routes; a key does not claim that a
single file is the only producer:

- **P0 — producerless legacy stub:** no ordinary HIR producer under
  `src/codegen/lir/hir_to_lir/`; current source construction is test-only.
- **PM — memory/address:** `expr/coordinator.cpp` global/local rvalues,
  `lvalue.cpp` lvalue load/store/addressing, `stmt.cpp` object/VLA storage, and
  `call/args.cpp` plus `call/vaarg*.cpp` ABI temporaries.
- **PE — expression/value:** `expr/binary.cpp`, `expr/misc.cpp`, `lvalue.cpp`,
  and `core.cpp` emit arithmetic, casts, comparisons, aggregate/vector ops,
  selects, and PHIs through `emit_lir_op`.
- **PC — call/intrinsic:** `call_args_ops.hpp::make_lir_call_op*`, called from
  `call/target.cpp`, `call/builtin.cpp`, and `call/vaarg.cpp`; typed intrinsic
  structs are emitted chiefly by `call/builtin.cpp`.
- **PS — statement/control adjunct:** `stmt.cpp` emits memset, stack restore,
  inline asm, and `LirIndirectBrOp`; `hir_to_lir.cpp` emits stack save.
- **PT — terminator:** `core.cpp::set_terminator_if_open` and the
  `emit_*br*`/`emit_ret`/`emit_switch` helpers; `hir_to_lir.cpp` supplies the
  synthesized zero return. There is no ordinary HIR producer for the legacy
  `LirIndirectBr` terminator.

Verifier keys refer to `src/codegen/lir/verify.cpp`:

- **V0:** `verify_inst` has no branch for any legacy stub; every field is
  ignored.
- **VK:** `LirOperand` kind/emptiness is checked, but text identity is not.
- **VT:** `LirTypeRef` or typed opcode/predicate form is checked.
- **VS:** shallow structural check only; the cell names what remains unchecked.
- **VG:** explicit verifier gap; the field/variant is ignored.

Coverage keys:

- **C-store:** focused `tests/backend/case/lir_identity_global_store.c` reaches
  `LirStoreOp` first; `global_store.c` remains integration evidence.
- **C-load:** focused `tests/backend/case/global_load.c` reaches `LirLoadOp`
  first; `defined_pointer_global_pointer.c` remains integration evidence.
- **C-array:** focused
  `tests/backend/case/lir_identity_global_array_address.c` reaches `LirGepOp`
  first with no preceding cast; `defined_global_array.c` remains integration
  evidence whose first instruction is `LirCastOp` and whose GEP is later.
- **C-array-integration:** `tests/backend/case/defined_global_array.c` reaches
  `LirCastOp` before its later `LirGepOp`.
- **C-ret:** focused `tests/backend/case/aarch64_return_zero_smoke.c` reaches
  non-void `LirRet`; despite its historical filename, the source and x86-64
  proof are target-neutral. `riscv64_zero_aggregate_global_storage.c` remains
  integration evidence.
- **C-call:** `tests/frontend/frontend_lir_call_type_ref_test.cpp` covers
  modern call type/signature mirrors and direct/indirect calls.
- **C-asm:** `tests/frontend/frontend_hir_tests.cpp` inline-asm cases and
  `tests/backend/bir/backend_lir_to_bir_interface_test.cpp` cover structured
  bindings and malformed roles/order.
- **C-const:** `backend_lir_to_bir_interface_test.cpp` constructs legacy
  constants; it is consumer coverage, not an ordinary HIR producer.
- **C-gap:** gap—Step 3 candidate; no focused identity-seam probe is bound.

## Step 3 focused probe bindings

These are expected failing producer probes. They establish the current first
fact; they do not claim BIR support.

| Focused probe | Exact primary contract | Step 4 carrier binding | Current baseline first failure | Integration contrast |
|---|---|---|---|---|
| `lir_identity_global_store.c` | `LirStoreOp.val` must gain native immediate identity for `7`; `LirStoreOp.ptr` must resolve the scalar global through stable symbol identity | [CC-STORE-1](carrier_contract.md#cc-store-1) | `UnsupportedOrdinaryInstruction`, `main`, `entry`; LLVM first body fact is `store i32 7, ptr @lir_identity_scalar` | `global_store.c` stores and then loads the same global; retain it as broader integration evidence |
| `global_load.c` | `LirLoadOp.result` needs stable result identity and `LirLoadOp.ptr` needs stable global-symbol identity | [CC-LOAD-1](carrier_contract.md#cc-load-1) | `UnsupportedOrdinaryInstruction`, `main`, `entry`; LLVM first body fact is `%t0 = load i32, ptr @g_counter` | `defined_pointer_global_pointer.c` adds pointer-global initialization/indexing and remains integration-only |
| `lir_identity_global_array_address.c` | First `LirGepOp` needs stable result/global-base identity and individually structured immediate indices `i64 0`, `i64 0` | [CC-GEP-1](carrier_contract.md#cc-gep-1) | `UnsupportedOrdinaryInstruction`, `main`, `entry`; LLVM first body fact is `%t0 = getelementptr [1 x i32], ptr @lir_identity_array, i64 0, i64 0`, with no preceding cast; later comparison/control is non-primary | `defined_global_array.c` remains cast-first/later-GEP integration evidence |
| `aarch64_return_zero_smoke.c` | `LirRet` needs typed native immediate identity for scalar `0` and typed `i32` return authority | [CC-RET-1](carrier_contract.md#cc-ret-1) | `InvalidVoidReturn`, `main`, `entry`; LLVM has no ordinary instruction and first body fact is `ret i32 0` | `riscv64_zero_aggregate_global_storage.c` remains aggregate/global integration evidence |

## `LirInst` alternatives

### Producerless legacy ID-backed alternatives

These rows are retained as the stable contrast: their value edges already use
`LirValueId`, but they have no ordinary HIR producer and `verify_inst` ignores
them completely.

| Variant + exact fields | Authority class | Producer route | Reachable verifier coverage | Focused probe/current coverage | Blocked idea-734 consumer relevance | Disposition |
|---|---|---|---|---|---|---|
| `LirConstInt.result` | `LirValueId` | P0 | V0 | C-const | Stable definition identity already consumable | `keep` |
| `LirConstInt.type`, `LirConstInt.value` | native/typed immediate (`TypeSpec`, `long long`) | P0 | V0 | C-const | Typed constant payload already consumable | `keep` |
| `LirConstFloat.result` | `LirValueId` | P0 | V0 | C-const | Stable definition identity already consumable | `keep` |
| `LirConstFloat.type`, `LirConstFloat.value` | native/typed immediate (`TypeSpec`, `double`) | P0 | V0 | C-const | Typed constant payload already consumable | `keep` |
| `LirLoad.result`, `LirLoad.ptr` | `LirValueId` | P0 | V0 | C-gap | Stable definition/use convention contrasts with modern load | `keep` |
| `LirLoad.type` | native/typed immediate (`TypeSpec`) | P0 | V0 | C-gap | Type metadata, not the blocked identity | `keep` |
| `LirStore.ptr`, `LirStore.val` | `LirValueId` | P0 | V0 | C-gap | Stable use convention contrasts with modern store | `keep` |
| `LirStore.type` | native/typed immediate (`TypeSpec`) | P0 | V0 | C-gap | Type metadata, not the blocked identity | `keep` |
| `LirBinary.result`, `LirBinary.lhs`, `LirBinary.rhs` | `LirValueId` | P0 | V0 | C-gap | Stable definition/use convention | `keep` |
| `LirBinary.type`, `LirBinary.op` | native/typed immediate (`TypeSpec`, `int`) | P0 | V0 | C-gap | Operation metadata, not focused identity | `keep` |
| `LirCast.result`, `LirCast.operand` | `LirValueId` | P0 | V0 | C-gap | Stable definition/use convention contrasts with modern cast | `keep` |
| `LirCast.from_type`, `LirCast.to_type` | native/typed immediate (`TypeSpec`) | P0 | V0 | C-gap | Type metadata, not focused identity | `keep` |
| `LirCmp.result`, `LirCmp.lhs`, `LirCmp.rhs` | `LirValueId` | P0 | V0 | C-gap | Stable definition/use convention | `keep` |
| `LirCmp.predicate` | native/typed immediate (`int`) | P0 | V0 | C-gap | Predicate metadata, not focused identity | `keep` |
| `LirCall.result`, `LirCall.callee_ptr`, `LirCall.args` | `LirValueId` | P0 | V0 | C-gap | Stable result/indirect-callee/argument convention | `keep` |
| `LirCall.return_type` | native/typed immediate (`TypeSpec`) | P0 | V0 | C-gap | Type metadata, not focused identity | `keep` |
| `LirCall.callee_name` | raw text | P0 | V0 | C-gap | Legacy direct-symbol identity has no `LinkNameId` | `extend existing ID/immediate convention` |
| `LirGep.result`, `LirGep.base_ptr`, `LirGep.indices` | `LirValueId` | P0 | V0 | C-gap | Stable definition/base/index value convention | `keep` |
| `LirGep.base_type` | native/typed immediate (`TypeSpec`) | P0 | V0 | C-gap | Type metadata, not focused identity | `keep` |
| `LirSelect.result`, `LirSelect.cond`, `LirSelect.true_val`, `LirSelect.false_val` | `LirValueId` | P0 | V0 | C-gap | Stable definition/use convention | `keep` |
| `LirSelect.type` | native/typed immediate (`TypeSpec`) | P0 | V0 | C-gap | Type metadata, not focused identity | `keep` |
| `LirIntrinsic.result`, `LirIntrinsic.args` | `LirValueId` | P0 | V0 | C-gap | Stable definition/use convention | `keep` |
| `LirIntrinsic.name` | raw text | P0 | V0 | C-gap | Legacy intrinsic selector is untyped and producerless | `extend existing ID/immediate convention` |
| `LirInlineAsm.result`, `LirInlineAsm.operands` | `LirValueId` | P0 | V0 | C-gap | Stable result/use convention | `keep` |
| `LirInlineAsm.asm_string`, `LirInlineAsm.constraints` | raw text | P0 | V0 | C-gap | Opaque assembly semantics, not an owned value/symbol/label seam | `not owned by focused seams` |

### Modern typed operations

`LirOperand` in every row below remains text plus kind classification. Calling
it “structured” does not make it a stable identity.

| Variant + exact fields | Authority class | Producer route | Reachable verifier coverage | Focused probe/current coverage | Blocked idea-734 consumer relevance | Disposition |
|---|---|---|---|---|---|---|
| `LirMemcpyOp.dst`, `LirMemcpyOp.src`, `LirMemcpyOp.size` | structured-but-text-only `LirOperand` | PM | VK | C-gap | Pointer/value uses lack stable identity/immediate payload | `extend existing ID/immediate convention` |
| `LirMemcpyOp.is_volatile` | native/typed immediate (`bool`) | PM | VG | C-gap | Semantic flag is already native | `keep` |
| `LirVaStartOp.ap_ptr` | structured-but-text-only `LirOperand` | PC | VK | C-gap | Pointer use lacks stable identity | `extend existing ID/immediate convention` |
| `LirVaEndOp.ap_ptr` | structured-but-text-only `LirOperand` | PC | VK | C-gap | Pointer use lacks stable identity | `extend existing ID/immediate convention` |
| `LirVaCopyOp.dst_ptr`, `LirVaCopyOp.src_ptr` | structured-but-text-only `LirOperand` | PC | VK | C-gap | Pointer uses lack stable identity | `extend existing ID/immediate convention` |
| `LirStackSaveOp.result` | structured-but-text-only `LirOperand` | PS | VK | C-gap | Result definition lacks stable identity | `extend existing ID/immediate convention` |
| `LirStackRestoreOp.saved_ptr` | structured-but-text-only `LirOperand` | PS | VK | C-gap | Pointer use lacks stable identity | `extend existing ID/immediate convention` |
| `LirAbsOp.result`, `LirAbsOp.arg` | structured-but-text-only `LirOperand` | PC | VK | C-gap | Definition/use edges lack stable identity or immediate payload | `extend existing ID/immediate convention` |
| `LirAbsOp.int_type` | native/typed immediate (`LirTypeRef`) | PC | VT | C-gap | Typed result/argument contract already exists | `keep` |
| `LirIndirectBrOp.addr` | structured-but-text-only `LirOperand` | PS | VK | C-gap | Address value lacks `LirValueId` | `extend existing ID/immediate convention` |
| `LirIndirectBrOp.targets` | raw text (`vector<string>` labels) | PS | VS: nonempty only; entries ignored | C-gap | CFG targets lack `LirBlockId` | `extend existing ID/immediate convention` |
| `LirExtractValueOp.result`, `LirExtractValueOp.agg` | structured-but-text-only `LirOperand` | PE | VK | C-gap | Definition/use edges lack stable identity | `extend existing ID/immediate convention` |
| `LirExtractValueOp.agg_type`, `LirExtractValueOp.index` | native/typed immediate (`LirTypeRef`, `int`) | PE | VT for `agg_type`; `index` is VG | C-gap | Type/index authority already structured | `keep` |
| `LirInsertValueOp.result`, `LirInsertValueOp.agg`, `LirInsertValueOp.elem` | structured-but-text-only `LirOperand` | PE | VK | C-gap | Definition/use/immediate edges lack stable identity | `extend existing ID/immediate convention` |
| `LirInsertValueOp.agg_type`, `LirInsertValueOp.elem_type`, `LirInsertValueOp.index` | native/typed immediate (`LirTypeRef`, `LirTypeRef`, `int`) | PE | VT for types; `index` is VG | C-gap | Type/index authority already structured | `keep` |
| `LirLoadOp.result`, `LirLoadOp.ptr` | structured-but-text-only `LirOperand` | PM | VK | C-load | Focused result/global-symbol boundary blocks load receipt | `extend existing ID/immediate convention` |
| `LirLoadOp.type_str` | native/typed immediate (`LirTypeRef`) | PM | VT | C-load | Type authority already structured | `keep` |
| `LirStoreOp.val`, `LirStoreOp.ptr` | structured-but-text-only `LirOperand` | PM | VK | C-store | Focused native-immediate/global-symbol boundary blocks store receipt | `extend existing ID/immediate convention` |
| `LirStoreOp.type_str` | native/typed immediate (`LirTypeRef`) | PM | VT | C-store | Type authority already structured | `keep` |
| `LirMemsetOp.dst`, `LirMemsetOp.byte_val`, `LirMemsetOp.size` | structured-but-text-only `LirOperand` | PM/PS | VK | C-gap | Pointer and immediate/value uses lack stable authority | `extend existing ID/immediate convention` |
| `LirMemsetOp.is_volatile` | native/typed immediate (`bool`) | PM/PS | VG | C-gap | Semantic flag already native | `keep` |
| `LirCastOp.result`, `LirCastOp.operand` | structured-but-text-only `LirOperand` | PE/PM | VK | C-array-integration | Integration definition/native-immediate operand boundary blocks cast receipt | `extend existing ID/immediate convention` |
| `LirCastOp.kind`, `LirCastOp.from_type`, `LirCastOp.to_type` | native/typed immediate (`LirCastKind`, `LirTypeRef`, `LirTypeRef`) | PE/PM | VT for types; kind is native but not separately checked | C-array-integration | Cast operation/type authority already structured | `keep` |
| `LirGepOp.result`, `LirGepOp.ptr` | structured-but-text-only `LirOperand` | PM/PE | VK | C-array | Focused result/global-or-value base boundary blocks GEP receipt | `extend existing ID/immediate convention` |
| `LirGepOp.indices` | raw text (`vector<string>` combined type/value fragments) | PM/PE | VG | C-array | Index values/immediates have no individual authority | `extend existing ID/immediate convention` |
| `LirGepOp.element_type`, `LirGepOp.inbounds` | native/typed immediate (`LirTypeRef`, `bool`) | PM/PE | VT for type; `inbounds` is VG | C-array | Type/flag authority already structured | `keep` |
| `LirCallOp.result`, `LirCallOp.callee` | structured-but-text-only `LirOperand` | PC | VK | C-call | Result and indirect-callee value identity remain text-only; direct callee text is not symbol authority | `extend existing ID/immediate convention` |
| `LirCallOp.direct_callee_link_name_id` | `LinkNameId` | PC (`call/target.cpp` supplies it for direct calls) | VG in `verify_inst` | C-call | Stable direct-call symbol contrast; already suitable for 734 | `keep` |
| `LirCallOp.return_type`, `LirCallOp.arg_type_refs`, `LirCallOp.return_ext_attr` | native/typed immediate (`LirTypeRef`, `vector<LirTypeRef>`, `LirExtAttr`) | PC | VT/parity checks | C-call | Typed call result/argument/extension metadata exists | `keep` |
| `LirCallOp.callee_type_suffix`, `LirCallOp.args_str` | raw text | PC formatter | VS: parsed for compatibility/parity | C-call | Compatibility rendering must cease being semantic authority | `presentation-only after structured authority` |
| `LirCallOp.callee_signature.return_type_ref`, `LirCallOp.callee_signature.return_ext_attr`, `LirCallOp.callee_signature.fixed_param_type_refs`, `LirCallOp.callee_signature.is_variadic`, `LirCallOp.callee_signature.has_unspecified_params`, `LirCallOp.callee_signature.has_void_param_list` | native/typed immediate (`LirTypeRef`/enum/bools) | PC (`call/target.cpp` signature builders) | VT and shape/parity checks | C-call | Structured signature authority exists | `keep` |
| `LirCallOp.callee_signature.fixed_param_types` | raw text | PC | VS: parity mirror for `fixed_param_type_refs` | C-call | Retained signature spelling only | `presentation-only after structured authority` |
| `LirCallOp.structured_args[].operand` | structured-but-text-only `LirOperand` | PC (`lir_call_structured_args`) | VG: verifier parses `args_str`, not this identity | C-call | Structured argument container still lacks value/immediate/symbol identity | `extend existing ID/immediate convention` |
| `LirCallOp.structured_args[].type` | raw text | PC | VG directly | C-call | Compatibility spelling beside `type_ref` | `presentation-only after structured authority` |
| `LirCallOp.structured_args[].type_ref`, `LirCallOp.structured_args[].aarch64_hfa_lane_count`, `LirCallOp.structured_args[].aarch64_hfa_lane_index`, `LirCallOp.structured_args[].aarch64_stack_align_bytes`, `LirCallOp.structured_args[].ext_attr` | native/typed immediate (`LirTypeRef`, sizes, enum) | PC | VG directly | C-call | Typed/ABI facts exist; they are not the identity gap | `keep` |
| `LirBinOp.result`, `LirBinOp.lhs`, `LirBinOp.rhs` | structured-but-text-only `LirOperand` | PE | VK | C-gap | Definition/use/immediate edges lack stable authority | `extend existing ID/immediate convention` |
| `LirBinOp.opcode`, `LirBinOp.type_str` | native/typed immediate (`LirBinaryOpcodeRef`, `LirTypeRef`) | PE | VT | C-gap | Opcode/type authority already structured | `keep` |
| `LirCmpOp.result`, `LirCmpOp.lhs`, `LirCmpOp.rhs` | structured-but-text-only `LirOperand` | PE | VK | C-gap | Definition/use/immediate edges lack stable authority | `extend existing ID/immediate convention` |
| `LirCmpOp.is_float`, `LirCmpOp.predicate`, `LirCmpOp.type_str` | native/typed immediate (`bool`, `LirCmpPredicateRef`, `LirTypeRef`) | PE | VT for predicate/type; `is_float` parity is shallow | C-gap | Predicate/type facts already structured | `keep` |
| `LirPhiOp.result` | structured-but-text-only `LirOperand` | PE | VK | C-gap | Definition identity lacks `LirValueId` | `extend existing ID/immediate convention` |
| `LirPhiOp.incoming[].first`, `LirPhiOp.incoming[].second` | raw text (value and label) | PE | VS: vector nonempty only; entries ignored | C-gap | Incoming value and predecessor label need stable value/block authority | `extend existing ID/immediate convention` |
| `LirPhiOp.type_str` | native/typed immediate (`LirTypeRef`) | PE | VT | C-gap | Type authority already structured | `keep` |
| `LirSelectOp.result`, `LirSelectOp.cond`, `LirSelectOp.true_val`, `LirSelectOp.false_val` | structured-but-text-only `LirOperand` | PE | VK | C-gap | Definition/use/immediate edges lack stable authority | `extend existing ID/immediate convention` |
| `LirSelectOp.type_str` | native/typed immediate (`LirTypeRef`) | PE | VT | C-gap | Type authority already structured | `keep` |
| `LirInsertElementOp.result`, `LirInsertElementOp.vec`, `LirInsertElementOp.elem`, `LirInsertElementOp.index` | structured-but-text-only `LirOperand` | PE | VK | C-gap | Definition/use/immediate edges lack stable authority | `extend existing ID/immediate convention` |
| `LirInsertElementOp.vec_type`, `LirInsertElementOp.elem_type` | native/typed immediate (`LirTypeRef`) | PE | VT | C-gap | Type authority already structured | `keep` |
| `LirExtractElementOp.result`, `LirExtractElementOp.vec`, `LirExtractElementOp.index` | structured-but-text-only `LirOperand` | PE | VK | C-gap | Definition/use/immediate edges lack stable authority | `extend existing ID/immediate convention` |
| `LirExtractElementOp.vec_type`, `LirExtractElementOp.index_type` | native/typed immediate (`LirTypeRef`) | PE | VT | C-gap | Type authority already structured | `keep` |
| `LirShuffleVectorOp.result`, `LirShuffleVectorOp.vec1`, `LirShuffleVectorOp.vec2`, `LirShuffleVectorOp.mask` | structured-but-text-only `LirOperand` | PE | VK | C-gap | Definition/use/constant-mask edges lack stable authority | `extend existing ID/immediate convention` |
| `LirShuffleVectorOp.vec_type`, `LirShuffleVectorOp.mask_type` | native/typed immediate (`LirTypeRef`) | PE | VT | C-gap | Type authority already structured | `keep` |
| `LirVaArgOp.result`, `LirVaArgOp.ap_ptr` | structured-but-text-only `LirOperand` | PM (`call/vaarg*.cpp`) | VK | C-gap | Definition/pointer-use edges lack stable identity | `extend existing ID/immediate convention` |
| `LirVaArgOp.type_str` | native/typed immediate (`LirTypeRef`) | PM | VT | C-gap | Type authority already structured | `keep` |
| `LirAllocaOp.result`, `LirAllocaOp.count` | structured-but-text-only `LirOperand` | PM | VK | C-gap | Definition/dynamic-count identity or immediate lacks stable authority | `extend existing ID/immediate convention` |
| `LirAllocaOp.type_str`, `LirAllocaOp.align` | native/typed immediate (`LirTypeRef`, `int`) | PM | VT for type; align is VG | C-gap | Type/alignment authority already structured | `keep` |
| `LirInlineAsmOp.result` | structured-but-text-only `LirOperand` | PS (`stmt.cpp` inline-asm lowering) | VK plus void/result parity | C-asm | Compatibility result is not semantic authority once ordinary bindings exist | `presentation-only after structured authority` |
| `LirInlineAsmOp.ret_type`, `LirInlineAsmOp.side_effects` | native/typed immediate (`LirTypeRef`, `bool`) | PS | VT and result parity; side-effect value not separately constrained | C-asm | Typed return/side-effect facts already exist | `keep` |
| `LirInlineAsmOp.asm_text`, `LirInlineAsmOp.constraints`, `LirInlineAsmOp.args_str` | raw text | PS | VS: compatibility constraint count participates; args/asm rendering shallow | C-asm | Explicit compatibility rendering | `presentation-only after structured authority` |
| `LirInlineAsmOp.original_asm_text`, `LirInlineAsmOp.original_constraint_text`, `LirInlineAsmOp.clobbers` | raw text | PS | VS: original constraints counted; asm/clobber bytes remain opaque | C-asm | Opaque semantic payload, not a value/symbol/label identity seam | `not owned by focused seams` |
| `LirInlineAsmOp.insn_r.opcode`, `LirInlineAsmOp.insn_r.funct3`, `LirInlineAsmOp.insn_r.funct7`, `LirInlineAsmOp.insn_r.operand_indices` | native/typed immediate (`uint32_t`, indices) | PS | VS: operand-index bounds only | C-asm | Native instruction metadata already structured | `keep` |
| `LirInlineAsmOp.ordinary_inputs[].value`, `LirInlineAsmOp.ordinary_results[].value` | structured-but-text-only `LirOperand` | PS | VK plus uniqueness/read-write pairing | C-asm | Semantic ordinary bindings still lack stable use/definition identity | `extend existing ID/immediate convention` |
| `LirInlineAsmOp.ordinary_inputs[].type`, `LirInlineAsmOp.ordinary_inputs[].role`, `LirInlineAsmOp.ordinary_inputs[].constraint_index`, `LirInlineAsmOp.ordinary_results[].type`, `LirInlineAsmOp.ordinary_results[].role`, `LirInlineAsmOp.ordinary_results[].constraint_index` | native/typed immediate (`LirTypeRef`, enum, index) | PS | VT plus role/order/pairing checks | C-asm | Binding type/position facts already structured | `keep` |

## `LirTerminator` alternatives

The stable terminator contrast is `LirIndirectBr`: unlike modern
`LirIndirectBrOp.targets` and every string-labelled terminator, it already owns
both a `LirValueId` address and `LirBlockId` targets. It is currently a
producerless legacy path and `verify_terminator` ignores it.

| Variant + exact fields | Authority class | Producer route | Reachable verifier coverage | Focused probe/current coverage | Blocked idea-734 consumer relevance | Disposition |
|---|---|---|---|---|---|---|
| `LirBr.target_label` | raw text | PT | VG | C-gap | CFG edge lacks `LirBlockId` | `extend existing ID/immediate convention` |
| `LirCondBr.cond_name` | raw text | PT | VS: converted to `LirOperand` for kind only | C-gap | Condition value/immediate lacks stable authority | `extend existing ID/immediate convention` |
| `LirCondBr.true_label`, `LirCondBr.false_label` | raw text | PT | VG | C-gap | CFG edges lack `LirBlockId` | `extend existing ID/immediate convention` |
| `LirRet.value_str` | raw text (`optional<string>`) | PT | VS: presence only | C-ret | Focused return value/immediate has no stable typed identity | `extend existing ID/immediate convention` |
| `LirRet.type_str` | raw text | PT | VS: empty/void presence parity only | C-ret | Return type is not a `LirTypeRef`; blocks exact 734 return receipt | `extend existing ID/immediate convention` |
| `LirSwitch.selector_name`, `LirSwitch.selector_type` | raw text | PT | VS: nonempty only | C-gap | Selector value and type lack stable authority | `extend existing ID/immediate convention` |
| `LirSwitch.default_label`, `LirSwitch.cases[].second` | raw text (labels) | PT | VG | C-gap | CFG targets lack `LirBlockId` | `extend existing ID/immediate convention` |
| `LirSwitch.cases[].first` | native/typed immediate (`long long`) | PT | VG | C-gap | Case value is already native | `keep` |
| `LirIndirectBr.addr` | `LirValueId` | producerless legacy terminator | VG | C-gap | Stable indirect address identity already suitable | `keep` |
| `LirIndirectBr.targets` | `LirBlockId` | producerless legacy terminator | VG | C-gap | Stable CFG target identities already suitable | `keep` |
| `LirUnreachable` (no fields) | native/typed immediate (closed variant tag) | PT default/open-block state | VG | Existing broad source coverage; no identity seam | No operand/result/symbol/label receipt needed | `not owned by focused seams` |

## Verifier and producer caveats

- `verify.cpp` ignores every legacy instruction stub, `LirBr`,
  `LirIndirectBr`, and `LirUnreachable`.
- Several modern checks validate only `LirOperandKind`; classification does not
  distinguish two SSA definitions with the same spelling, resolve a global to
  `LinkNameId`, or separate a typed immediate payload from its text.
- GEP `indices`, PHI incoming values/labels, `LirIndirectBrOp.targets`, and all
  terminator labels remain raw text. `LirRet` remains only optional
  `value_str` plus `type_str`.
- `LirCallOp.result`, `callee`, and `structured_args[].operand` remain
  text-only even though direct calls have stable
  `direct_callee_link_name_id`. Its `args_str` and `callee_type_suffix` are
  still parsed for compatibility checks.
- Inline-asm ordinary bindings are semantic, but their `value` members remain
  `LirOperand` text identities. Assembly templates, constraints, and clobbers
  are intentionally opaque and are not candidates for value-ID conversion.
- The four focused probes bind only store, load, GEP, and return seams. The
  original four larger cases remain integration evidence, including the exact
  cast-first/later-GEP fact for `defined_global_array.c`. Every other unbound
  owned identity row is explicitly marked `C-gap—Step 3 candidate`; no coverage
  is inferred from mere variant presence.

## Checked conclusion

The smallest candidate family for the owned blocked rows is to extend the
existing ordinary conventions already present in this same LIR model:

- `LirValueId` for produced and consumed values,
- `LinkNameId` for semantic direct/global symbol identity,
- `LirBlockId` for CFG labels and predecessor/target edges, and
- native typed immediate payloads for constants and indices.

Display strings may remain only for presentation or exact parity after those
structured authorities exist. The matrix provides no evidence for a parallel
value/symbol model and deliberately does not prescribe implementation design.

## Mechanical inventory result

The `LirInst` alternative list contains **38** names:

`LirConstInt`, `LirConstFloat`, `LirLoad`, `LirStore`, `LirBinary`, `LirCast`,
`LirCmp`, `LirCall`, `LirGep`, `LirSelect`, `LirIntrinsic`, `LirInlineAsm`,
`LirMemcpyOp`, `LirVaStartOp`, `LirVaEndOp`, `LirVaCopyOp`, `LirStackSaveOp`,
`LirStackRestoreOp`, `LirAbsOp`, `LirIndirectBrOp`, `LirExtractValueOp`,
`LirInsertValueOp`, `LirLoadOp`, `LirStoreOp`, `LirMemsetOp`, `LirCastOp`,
`LirGepOp`, `LirCallOp`, `LirBinOp`, `LirCmpOp`, `LirPhiOp`, `LirSelectOp`,
`LirInsertElementOp`, `LirExtractElementOp`, `LirShuffleVectorOp`, `LirVaArgOp`,
`LirAllocaOp`, and `LirInlineAsmOp`.

The `LirTerminator` alternative list contains **6** names:

`LirBr`, `LirCondBr`, `LirRet`, `LirSwitch`, `LirIndirectBr`, and
`LirUnreachable`.
