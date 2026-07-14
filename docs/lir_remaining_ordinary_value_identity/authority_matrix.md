# Remaining Ordinary LIR Value-Identity Authority Matrix

Checked against current source at `d0aa7c0d9`. This is the idea-744 Plan Step 1
execution baseline. It inventories all 38 current `LirInst` alternatives, plus
the adjacent terminator and non-instruction identity families, without claiming
producer capability. The four closed idea-741 contracts are regression
neighbors only:

- CC-STORE-1: selected-global integer-immediate `LirStoreOp`;
- CC-LOAD-1: selected-global scalar `LirLoadOp`;
- CC-GEP-1: selected-global array-decay `LirGepOp`;
- CC-RET-1: scalar integer/void `LirRet`.

## Evidence and terminology

`LirOperand::authority()` is exactly `monostate | LirValueId | LinkNameId |
LirIntegerImmediate`. `fresh_tmp(ctx)` allocates only display spelling;
`fresh_value(ctx)` first calls the owning `LirFunction::alloc_value()` and then
returns `LirOperand::ssa(display, id)`. Current production `fresh_value` calls
cover CC-LOAD-1 and CC-GEP-1 in `StmtEmitter::emit_rval_operand`, plus the
Step-3 structured direct integer-call result in `emit_call_with_result` and the
Step-6 ordinary scalar integer and Step-7.5 ordinary scalar floating arithmetic
branches in `emit_binary_rval_operand`, plus the Step-7.1 explicit scalar
integer, Steps 7.7/7.8 explicit scalar FPTrunc/FPExt casts, Steps 7.9/7.10
explicit scalar SIToFP/UIToFP casts, and the Steps 7.11/7.12 explicit scalar
FPToSI/FPToUI casts in `emit_cast_rval_operand`, plus the Step-7.13 wide
builtin-ffs select narrowing cast, Step-7.14 shared ffs add-one result,
Step-7.15 shared ffs zero-comparison result, and Step-7.16 shared ffs cttz call
result, plus the Step-7.17 builtin-ctz i32/i64 cttz call and i64 narrowing
result, plus the Step-7.18 builtin-clz i32/i64 ctlz call and i64 narrowing
result, plus the Step-7.19 builtin-popcount i32/i64 ctpop call and i64
narrowing result, plus the Step-7.21 scalar integer output-only inline-asm
semantic result, plus the Step-7.2 integer / Step-7.6 floating ordinary
scalar compare branches in `emit_binary_rval_operand`, plus the Step-7.3 scalar builtin-ffs select in
`emit_builtin_ffs_call` and the Step-7.4 integer builtin-abs result in
`emit_post_builtin_call_operand`. Other active modern result constructions
identified below still use `fresh_tmp`, an equivalent direct `%t` increment,
or a raw string returned by `emit_rval_id`.

Verifier states used below:

- **none**: the legacy alternative is not dispatched by `verify_inst`.
- **kind/type**: operand kind and native type/opcode shape are checked, but
  monostate/text operands pass and no definition/use identity is proven.
- **ownership-ready**: `verify_function_value_ownership` already collects a
  native result ID and resolves native use IDs when producers populate them.
- **741 exact**: the closed contract adds its exact global/immediate/value
  ownership rules; no neighboring shape is implied.

Receiver dispositions are exact: **regression neighbor**, **generic value**,
**distinct carrier**, **separate family**, **presentation/opaque**, or
**producerless**.

## Producer map

These names are current construction functions, not inferred subsystem labels.

| Code | Exact current producer function and file |
|---|---|
| P0 | No construction reference under `src/`; declaration and variant membership only in `src/codegen/lir/ir.hpp` |
| PR | `StmtEmitter::emit_rval_operand` / `emit_rval_id`, `src/codegen/lir/hir_to_lir/expr/coordinator.cpp` |
| PB | `StmtEmitter::emit_rval_payload(BinaryExpr)` and `emit_logical`, `src/codegen/lir/hir_to_lir/expr/binary.cpp` |
| PX | `StmtEmitter::emit_rval_payload(UnaryExpr|CastExpr|TernaryExpr|IndexExpr|MemberExpr)`, `src/codegen/lir/hir_to_lir/expr/misc.cpp` |
| PL | `StmtEmitter::emit_member_gep`, `emit_bitfield_load/store`, `emit_lval_dispatch`, `emit_load/store_assignable_value`, `emit_nonptr_compound_assign_value`, `emit_indexed_gep`, and `emit_rval_from_access_ptr`, `src/codegen/lir/hir_to_lir/lvalue.cpp` |
| PC | `StmtEmitter::prepare_call_arg` / `prepare_call_args`, `call/args.cpp`; `emit_void_call` / `emit_call_with_result`, `call/target.cpp`; `lir_call_structured_args` / `make_lir_call_op_with_return_type_ref`, `src/codegen/lir/call_args_ops.hpp` |
| PI | `StmtEmitter::emit_builtin_*` and `emit_post_builtin_call`, `src/codegen/lir/hir_to_lir/call/builtin.cpp` |
| PV | `StmtEmitter::emit_rval_payload(VaArgExpr)`, `emit_aarch64_vaarg_*`, `call/vaarg.cpp`; `emit_amd64_va_arg*`, `call/vaarg_amd64.cpp`; register helpers in `call/vaarg_amd64_registers.cpp` |
| PS | `StmtEmitter::emit_non_control_flow_stmt(LocalDecl|InlineAsmStmt)` and `emit_control_flow_stmt(GotoStmt|IndirBrStmt)`, `src/codegen/lir/hir_to_lir/stmt.cpp` |
| PO | `StmtEmitter::coerce` and `to_bool`, `src/codegen/lir/hir_to_lir/core.cpp` |
| PF | `init_fn_ctx` and `hoist_allocas`, `src/codegen/lir/hir_to_lir/hir_to_lir.cpp` |

## Mechanical 38-alternative matrix

Each alternative appears exactly once in the first column. Field families are
grouped only where they share one producer and disposition.

| `LirInst` alternative and field families | Production and exact producer | Current carrier and result allocation | Native semantic authority and reachable verifier | Dependency, focused probe, exact receiver disposition |
|---|---|---|---|---|
| `LirConstInt`: `result`; `type,value` | Producerless legacy, P0 | `LirValueId`; `TypeSpec,long long`; no production allocation | Native ID/type/value, verifier none | No probe; **producerless**. Existing bounded new-BIR constant receipt is not producer evidence |
| `LirConstFloat`: `result`; `type,value` | Producerless legacy, P0 | `LirValueId`; `TypeSpec,double`; no production allocation | Native ID/type/value, verifier none | No probe; **producerless** |
| `LirLoad`: `result,ptr`; `type` | Producerless legacy, P0 | `LirValueId`; `TypeSpec`; no production allocation | Native IDs/type, verifier none | No probe; **producerless** legacy contrast |
| `LirStore`: `ptr,val`; `type` | Producerless legacy, P0 | `LirValueId`; `TypeSpec`; no result | Native IDs/type, verifier none | No probe; **producerless** legacy contrast |
| `LirBinary`: `result,lhs,rhs`; `type,op` | Producerless legacy, P0 | `LirValueId`; `TypeSpec,int`; no production allocation | Native IDs/type/op, verifier none | No probe; **producerless** legacy contrast for PB |
| `LirCast`: `result,operand`; `from_type,to_type` | Producerless legacy, P0 | `LirValueId`; `TypeSpec`; no production allocation | Native IDs/types, verifier none | No probe; **producerless** |
| `LirCmp`: `result,lhs,rhs`; `predicate` | Producerless legacy, P0 | `LirValueId`; native `int`; no production allocation | Native IDs/predicate, verifier none | No probe; **producerless** |
| `LirCall`: `result,callee_ptr,args`; `return_type`; `callee_name` | Producerless legacy, P0 | `LirValueId`; `TypeSpec`; raw direct name; no allocation | Native value IDs but no `LinkNameId`; verifier none | No probe; **producerless**; raw direct name cannot become authority |
| `LirGep`: `result,base_ptr,indices`; `base_type` | Producerless legacy, P0 | `LirValueId`; `TypeSpec`; no production allocation | Native IDs/type, verifier none | No probe; **producerless** |
| `LirSelect`: `result,cond,true_val,false_val`; `type` | Producerless legacy, P0 | `LirValueId`; `TypeSpec`; no production allocation | Native IDs/type, verifier none | No probe; **producerless** |
| `LirIntrinsic`: `result,args`; `name` | Producerless legacy, P0 | `LirValueId`; raw selector; no production allocation | Native value IDs, untyped selector, verifier none | No probe; **producerless**; selector would need a **distinct carrier** |
| `LirInlineAsm`: `result,operands`; `asm_string,constraints` | Producerless legacy, P0 | `LirValueId`; opaque raw strings; no allocation | Native value IDs; verifier none | No probe; **producerless**; text remains **opaque** |
| `LirMemcpyOp`: `dst,src,size`; `is_volatile` | Active in PL (`emit_lval_dispatch`), PC (`prepare_call_arg`), PI (`emit_post_builtin_call`), PV (`emit_aarch64_vaarg_hfa`, `emit_rval_payload(VaArgExpr)`, `emit_amd64_va_arg_from_overflow`, AMD64 register helper), and PO (`coerce`) | All operands are text-only/monostate `LirOperand`; no result; sources are generally `emit_rval_id`/`fresh_tmp` | Native bool; verifier kind only, ownership-ready if IDs are populated | Depends on pointer/object authority and generic scalar uses; proposed `lir_memcpy_native_use_identity.c`; **separate memory/object family** until pointer ownership exists |
| `LirVaStartOp`: `ap_ptr` | Active, PI `emit_post_builtin_call` | Text-only/monostate `LirOperand` from `emit_va_list_obj_ptr`; no result | Verifier pointer kind only | `lir_vastart_pointer_identity.c`; **separate va-list/object family** |
| `LirVaEndOp`: `ap_ptr` | Active, PI `emit_post_builtin_call` | Text-only/monostate `LirOperand`; no result | Verifier pointer kind only | `lir_vaend_pointer_identity.c`; **separate va-list/object family** |
| `LirVaCopyOp`: `dst_ptr,src_ptr` | Active, PI `emit_post_builtin_call` | Text-only/monostate `LirOperand`; no result | Verifier pointer kinds only | `lir_vacopy_pointer_identity.c`; **separate va-list/object family** |
| `LirStackSaveOp`: `result` | Active, PF `lower_function` VLA setup | Text-only/monostate `LirOperand`; direct `%t` increment, equivalent to `fresh_tmp` | Result kind only; ownership-ready once populated | `lir_vla_stacksave_identity.c`; **separate stack/local family**, despite generic result shape |
| `LirStackRestoreOp`: `saved_ptr` | Active, PS `emit_control_flow_stmt(GotoStmt)` | Text-only/monostate `LirOperand` copied from saved display | Pointer kind only | Depends on stack-save identity; `lir_vla_stackrestore_identity.c`; **separate stack/local family** |
| `LirAbsOp`: `result,arg`; `int_type` | Active, PI `emit_post_builtin_call_operand` for integer `abs`/`labs`/`llabs` | Step-7.4 uses `fresh_value`, exact integer type, and the common operand/coercion path; unchanged selected-global SSA and integer-immediate arguments retain native authority, while structurally unavailable sources remain honest monostate SSA compatibility | Authoritative integer abs requires a native result, exact integer type, and SSA/immediate argument shape; generic ownership rejects invalid/duplicate results and unknown/cross-function uses | `lir_scalar_abs_result_use_identity.c` closes the current integer builtin route and its later ordinary use; other call families and noninteger/aggregate/vector routes remain unclaimed |
| `LirIndirectBrOp`: `addr`; `targets` | Active, PS `emit_control_flow_stmt(IndirBrStmt)` | Address text-only from `emit_rval_id`; targets raw `vector<string>` | Address kind plus nonempty target vector; no target identity | Address could use generic value, but targets require `LirBlockId`; `lir_indirectbr_identity.c`; **separate CFG family** |
| `LirExtractValueOp`: `result,agg`; `agg_type,index` | Active in PX unary/cast paths, PB `emit_complex_binary_arith` / `emit_rval_payload(BinaryExpr)`, PI complex/overflow builtins, and PO `coerce` | Value operands text-only; result `fresh_tmp` | Native `LirTypeRef,int`; kind/type only and ownership-ready | Aggregate type/index validation is additional; `lir_extractvalue_chain_identity.c`; **generic result/use plus distinct aggregate semantics** |
| `LirInsertValueOp`: `result,agg,elem`; `agg_type,elem_type,index` | Active in PX unary paths, PB `emit_complex_binary_arith` / `emit_rval_payload(BinaryExpr)`, PI complex builtins, and PO `coerce` | Value operands text-only; result `fresh_tmp` | Native types/index; kind/type only and ownership-ready | `lir_insertvalue_chain_identity.c`; **generic result/use plus distinct aggregate semantics** |
| `LirLoadOp`: `result,ptr`; `type_str` | Active broadly in PR/PL/PX/PC/PV/PO; CC-LOAD-1 exact producer is PR selected-global branch | CC-LOAD-1 uses `fresh_value` result + global `LinkNameId`; all local/SSA/object routes use text-only operands and `fresh_tmp` | Native `LirTypeRef`; kind/type + ownership-ready; **741 exact** only for selected global | Preserve CC-LOAD-1 as **regression neighbor**. Other routes depend on local/object pointer ownership; probes per route, starting `lir_local_load_identity.c`; **separate pointer/object family** |
| `LirStoreOp`: `val,ptr`; `type_str` | Active in PL/PX/PC/PI/PV/PS/PO and PF parameter/local setup; CC-STORE-1 exact producer is PL `emit_set_assign_value` via `integer_store_operand_after_coercion` | CC-STORE-1 has native immediate + global `LinkNameId`; other routes text-only/monostate; no result | Native `LirTypeRef`; kind/type + ownership-ready; **741 exact** only for selected global integer | Preserve CC-STORE-1 as **regression neighbor**. SSA value use can share generic propagation, pointer needs object authority; `lir_local_store_identity.c`; **mixed generic/separate** |
| `LirMemsetOp`: `dst,byte_val,size`; `is_volatile` | Active, PL `emit_store_assignable_value` zero aggregate and PS `emit_non_control_flow_stmt(LocalDecl)` | Text-only/monostate operands; no result | Native bool; verifier kinds only | `lir_memset_native_use_identity.c`; **separate memory/object family** |
| `LirCastOp`: `result,operand`; `kind,from_type,to_type` | Active in PX/PB/PL/PC/PI/PV/PO and PF fixed-vector parameter setup; Steps 7.1 and 7.7-7.12 own PX explicit scalar cast routes, Step 7.13 owns PI's wide builtin-ffs select narrowing, and Steps 7.17-7.19 own only PI's builtin-ctzll/clzll/popcountll result narrowing | The focused routes use `fresh_value`, exact input `LirOperand`, and the operand-returning coercion wrapper. Steps 7.13 and 7.17-7.19 reuse the integer seam for authoritative i64 sources and exact i64-to-i32 Trunc results; other builtin narrowing and cast producers remain text-only with `fresh_tmp` | Native kind and exact endpoints; authoritative integer casts require coherent widths, floating casts and conversions require exact endpoint families, and generic ownership rejects invalid/duplicate definitions and unknown/cross-function uses | The focused scalar cast probes close the representative PX routes plus only the wide ffs select-to-Trunc-to-use and ctz/clz/popcount call-to-Trunc-to-use chains. Pointer, bitcast, vector, aggregate, implicit coercion, other builtins, and other producers remain unclaimed |
| `LirGepOp`: `result,ptr,indices`; `element_type,inbounds` | Active in PR/PL/PX/PC/PV and PF parameter setup; CC-GEP-1 exact producer is PR selected-global array branch | CC-GEP-1 uses `fresh_value`, global `LinkNameId`, typed native indices; other routes use `fresh_tmp`, raw base, often raw `LirGepIndex` presentation | Native type/bool; ownership-ready; **741 exact** only for selected-global typed path | Preserve CC-GEP-1 as **regression neighbor**. Other paths require object/base and typed-index work; `lir_local_gep_identity.c`; **separate pointer/object family** |
| `LirCallOp`: `result`; `callee,direct_callee_link_name_id`; `intrinsic_kind,zero_count_behavior`; `structured_args[].operand`; typed signature/type/ext/ABI fields; text mirrors | Active in PC and PI; common builders include `make_lir_call_op_with_return_type_ref` | Structured direct integer results use `fresh_value`; fixed integer arguments use `OwnedLirTypedCallArg`. Steps 7.16-7.19 publish PI's native Cttz/Ctlz/Ctpop kinds, i32/i64 ffs/ctz/clz/popcount results, module `LinkNameId` callees, exact fixed integer signatures and refs, and authority-free prepared SSA/immediate presentation. Cttz/Ctlz carry their exact zero behavior and flag; Ctpop has one integer parameter and no zero-count behavior | Direct integer results have exact ownership. The fixed void and native intrinsic rows have authority-first kind/callee/signature/type/count/ext/argument validation; zero-count behavior must exactly agree for Cttz/Ctlz and be absent for Ctpop. Authoritative prepared values must be current-function SSA, while monostate literal presentation stays compatible | Steps 3-5 and 7.16-7.19 close scalar result, fixed void arguments, the ffs cttz result-to-add edge, and ctz/ctlz/ctpop call-to-optional-Trunc-to-i32-use edges. Other intrinsic, indirect, variadic, ABI, aggregate, and object rows remain unclaimed; text mirrors are presentation-only |
| `LirBinOp`: `result,lhs,rhs`; `opcode,type_str` | Active, PB scalar/complex arithmetic and logical helpers; also PL compound assignment, PI builtins, PV, PS | Step-6 ordinary scalar integer, Step-7.5 ordinary scalar floating arithmetic, and Step-7.14 PI i32/i64 ffs add-one use `fresh_value`. Ordinary integer immediates retain authority only when representable by the normalized operation type; Step 7.16 supplies the exact cttz result ID as the ffs lhs, while exact immediate one and the add-one-to-select edge remain authoritative | Native opcode/type refs plus exact ownership; authoritative floating opcode/type alternatives agree, and authoritative integer operand alternatives are SSA or representable integer immediates when present. Generic ownership rejects invalid/duplicate definitions and unknown/cross-function uses | The ordinary scalar probes plus the focused i32/i64 ffs add-one/cttz probes close only those rows. Converted modulo/bit-pattern literals remain compatibility operands. Other builtins, complex/vector, pointer/object, logical-helper, and other producers remain distinct |
| `LirCmpOp`: `result,lhs,rhs`; `is_float,predicate,type_str` | Active, PB comparisons/logical, PI FP/builtin checks, PV, PS loop/range lowering, `core.cpp` helpers; Steps 7.2 and 7.6 own PB ordinary scalar integer and floating comparisons, while Step 7.15 owns PI's shared i32/i64 ffs equality-to-zero row | The PB rows use `fresh_value` and feed the result into normalization. Step 7.15 uses `fresh_value`, native Eq/exact integer type, an honest monostate prepared argument, exact immediate zero, and the result ID as the select condition. Other comparison producers remain text-only | Native mode/predicate/type authority must agree; the authoritative integer select-condition edge accepts only SSA or representable integer-immediate comparison operands when authority is present. Generic ownership rejects invalid/duplicate definitions and unknown/cross-function uses | Focused probes close the two PB rows and only the shared ffs zero-comparison-to-condition edge. Other builtin, pointer, vector, complex/logical-helper, vaarg, and statement producers remain unclaimed |
| `LirPhiOp`: `result`; `incoming[value,label]`; `type_str` | Active, PX `emit_rval_payload(TernaryExpr)`, PB `emit_logical`, PV AArch64/AMD64 joins | Result `fresh_tmp`; incoming value and predecessor are raw strings | Native result type only; result kind/nonempty incoming; incoming entries are not visited for value ownership | Result can share generic allocation, but incoming values need `LirOperand` and predecessors need `LirBlockId`; `lir_phi_identity.c`; **distinct value+CFG carrier** |
| `LirSelectOp`: `result,cond,true_val,false_val`; `type_str` | Active only in PI `emit_builtin_ffs_call` | Step-7.3 i32 and Step-7.13 i64 ffs routes use `fresh_value`, exact scalar type, and native zero immediate; Step 7.14 preserves add-one as the false arm, Step 7.15 preserves equality-to-zero as the condition, and Step 7.16 supplies the add-one's exact cttz lhs | Authoritative integer select requires a native result and an SSA condition defined by a current-function comparison; generic ownership rejects invalid/duplicate definitions and unknown/cross-function edges | Focused probes close direct i32 select use, i64 select → Trunc → use, and the shared cttz → add-one → false-arm plus comparison → condition chain. Other producers remain unclaimed |
| `LirInsertElementOp`: `result,vec,elem,index`; `vec_type,elem_type` | Active, PB vector-scalar arithmetic branches | Text-only operands; result `fresh_tmp` | Native types; kind/type only and ownership-ready | `lir_insertelement_identity.c`; **generic result/use plus distinct vector/index semantics** |
| `LirExtractElementOp`: `result,vec,index`; `vec_type,index_type` | Active, PX `emit_rval_payload(IndexExpr)` vector branch | Text-only operands; result `fresh_tmp` | Native types; kind/type only and ownership-ready | `lir_extractelement_identity.c`; **generic result/use plus distinct vector/index semantics** |
| `LirShuffleVectorOp`: `result,vec1,vec2,mask`; `vec_type,mask_type` | Active, PB vector-scalar splat branches | Text-only operands; result `fresh_tmp` | Native types; kind/type only and ownership-ready | `lir_shufflevector_identity.c`; mask needs distinct typed constant/vector carrier; **generic result/use plus distinct vector semantics** |
| `LirVaArgOp`: `result,ap_ptr`; `type_str` | Active, PV `emit_rval_payload(VaArgExpr)` and `emit_amd64_va_arg` semantic routes | Text-only operands; result `fresh_tmp` | Native type; kind/type only and ownership-ready | `lir_vaarg_identity.c`; **generic result** but pointer/object/ABI use is **separate va-list family** |
| `LirAllocaOp`: `result,count`; `type_str,align` | Active in PF/PS/PL/PX/PC/PI/PV/`core.cpp` for VLA, locals, temporaries and ABI copies | Result and optional count text-only; result `fresh_tmp` or named stack spelling | Native type/alignment; kind/type only and ownership-ready | Requires stack/local object ownership, dynamic-count use, and hoisted/body ordering; `lir_alloca_object_identity.c`; **separate stack/local family** |
| `LirInlineAsmOp`: compatibility `result`; semantic `ordinary_inputs/results[].value`; binding type/role/index; original text/clobbers; rendered mirrors; `insn_r` | Active, PS `emit_non_control_flow_stmt(InlineAsmStmt)` | Steps 7.21 and 7.25 prove the width-generic scalar integer output-only path for i32 and i64: the semantic result is allocated through `fresh_value`, the compatibility result stays authority-free, and the exact binding ID reaches the type-matched Store. Read/write, memory, multi-output, and other bindings remain compatibility. Assembly/constraint/clobber text remains raw by design | Native binding types/roles/indices and R metadata; verifier checks shape/order/pairing. The focused output-only rows require one exact type/role/index result binding, collect it as a function-owned definition, and require its Store use type to match at both proven widths | Steps 7.21 and 7.25 close only the scalar i32/i64 output-only binding-to-Store edges. Read/write/input/memory/vector/multi-output/explicit-register/`insn_r` semantics and all opaque text remain unclaimed or presentation-only; `lir_inline_asm_binding_identity.c`; **generic binding values + presentation/opaque payload** |

## Separate adjacent identity families

These are not hidden ordinary prerequisites.

| Family | Current source fact | Disposition |
|---|---|---|
| CFG/terminators | `LirBr`, `LirCondBr`, `LirSwitch`, `LirPhiOp.incoming[].second`, and active `LirIndirectBrOp.targets` use raw labels. Producerless legacy `LirIndirectBr` alone has `LirBlockId` targets. | Separate CFG carrier initiative/packet; never infer IDs from labels |
| Stack/local/alloca/object | `LirStackObject` owns `LirStackSlotId`, but `LirAllocaOp.result`, local addresses, lifetime-like VLA save/restore, and most local load/store/GEP pointers are raw spellings without a joined object/value contract. | Separate stack/local ownership; generic result IDs alone are insufficient |
| Inline-asm opaque text | `original_asm_text`, `original_constraint_text`, `clobbers`, and rendered mirrors are ordered opaque bytes. | Never convert to value IDs; only ordinary binding values join generic publication |
| Body parameters | `LirFunction.params` and `signature_params` carry names/types/ordinals, not function-owned `LirValueId`; `verify_function_value_ownership` has no parameter-definition registry. | Separate body-parameter publication; names and ABI positions are not identities |
| Terminator return neighbor | CC-RET-1 uses native `LirIntegerImmediate` or current-function `LirValueId` plus `LirTypeRef`; other return value types remain unclaimed. | Preserve as 741 regression neighbor; non-integer returns need their own semantic/type packet |

## Generic groups and distinct carriers

The common result allocator can cover `LirCallOp`, `LirBinOp`, `LirCmpOp`,
`LirCastOp`, `LirSelectOp`, `LirAbsOp`, aggregate/vector results, and semantic
inline-asm outputs: allocate with `fresh_value(ctx)`, store the same
`LirOperand`, and let `verify_function_value_ownership` enforce uniqueness and
known current-function uses. It does not by itself solve:

- call argument preparation beyond the Step-4 immediate and Step-5
  selected-global SSA rows: the common `LirOperand` carrier exists, but other
  SSA sources and ABI/aggregate transforms retain phased monostate compatibility;
- PHI predecessor identity and active indirect-branch targets (`LirBlockId`);
- alloca/stack-slot/local-object ownership (`LirStackSlotId` plus value/address
  relation);
- aggregate/vector opcode-specific type/index/mask rules;
- va-list and memory intrinsic pointer/object/ABI contracts;
- body parameter definitions; or opaque inline-assembly text.

## Step-2 seam executability

All four required focused probes are independently extractable from current
production; there is no contradictory larger prerequisite.

1. **Direct scalar call result**: completed in Step 3. The producer allocates
   through `fresh_value`, the maker accepts the native operand, and the
   CallExpr operand path preserves it through the downstream return use.
2. **Void immediate argument**: completed in Step 4. `prepare_call_arg`
   obtains the native operand, retains its payload only for the fixed
   nonvariadic integer path with unchanged representation, and the common
   operand carrier survives formatting and structured-argument construction.
3. **Void SSA argument**: completed in Step 5. The selected-global load returns
   the CC-LOAD-1 `LirValueId`, and `prepare_call_arg` preserves that exact
   operand through the common carrier introduced for seam 2 when the fixed
   integer representation is unchanged.
4. **Non-call scalar chain**: completed in Step 6. PB allocates normalized
   ordinary scalar integer `LirBinOp` results through `fresh_value`, returns the
   native operand through `emit_binary_rval_operand`, and preserves an input's
   authority only when coercion leaves its representation unchanged. The
   two-operation chain proves exact result/use propagation without calls, CFG,
   locals, parameters, or ABI work.

Thus Step 2 may proceed. Seam 3 reused the common call-argument carrier
established by seam 2 without adding an SSA-only carrier; the dependency was
sequencing, not a hidden prerequisite.

## Step-2 focused probe bindings

These probes record the implemented production boundaries. The matching
structural observations live in
`frontend_lir_call_type_ref_test.cpp` and select LIR alternatives directly;
rendered LLVM output is not used as identity evidence.

| Probe and stable first bad fact | Producer and exact carrier transition | Required verifier rejection after publication | Forbidden fallback |
|---|---|---|---|
| `lir_direct_scalar_result_call_identity.c`: Step-2 first bad fact closed; the sole structured direct i32 `LirCallOp.result` and its return use now carry the same valid `LirValueId` | PC `emit_call_with_result`: `fresh_value(ctx)` -> operand-taking `make_lir_call_op_with_return_type_ref(LirOperand)`; `emit_rval_call_operand` returns the identical operand to the existing return path. Direct target `LinkNameId`, native i32 return ref, and structured signature remain unchanged | Implemented: reject missing/invalid/duplicate result IDs, unknown/cross-function downstream IDs, and any void-result authority through exact call checks plus generic current-function ownership | `%tN`, call formatting, callee spelling, and instruction order remain presentation only; misleading producer/use displays with the same ID verify successfully |
| `lir_direct_void_immediate_arg_identity.c`: Step-2 first bad fact closed; the sole fixed i32 structured argument carries `LirIntegerImmediate{7}`, exact i32 type refs, `None` extension, and no call result | PR `emit_rval_operand` -> PC representation-preserving fixed integer coercion -> `OwnedLirTypedCallArg::operand: LirOperand` -> authority-preserving `lir_call_structured_args`; formatting reads only `operand.str()` | Implemented authority-first rejection for missing payload, wrong/invalid alternatives, range, type/signature/count/ext conflicts; complete structured authority ignores misleading argument/type presentation | `operand`, `args_str`, raw argument/parameter type spellings, and suffix text are never reparsed to create or repair authority |
| `lir_direct_void_ssa_arg_identity.c`: Step-2 first bad fact closed; CC-LOAD-1 and the sole fixed i32 structured argument carry the same valid current-function `LirValueId` | PR selected-global `emit_rval_operand` returns authoritative `LirOperand`; PC recognizes the structural global `DeclRef`, preserves the exact source operand through the common call-argument carrier, and supplies exact i32 refs only for the unchanged fixed integer representation | Implemented: reject monostate, wrong/invalid authority, unknown/cross-function IDs, and type/signature/count/ext conflicts; generic ownership requires the use ID to resolve to the existing current-function load definition while CC-LOAD-1 verification remains unchanged | Load/result and argument displays, `%tN`, `args_str`, suffix text, and raw type mirrors never select or repair identity; misleading mirrors with the same ID verify successfully |
| `lir_scalar_ordinary_value_chain_identity.c`: Step-2 first bad fact closed; both PB binary results carry distinct valid IDs and the second operation's lhs carries the exact first result ID | PB `emit_binary_rval_operand`: normalized scalar integer arithmetic uses `fresh_value`; unchanged source operands retain their common `LirOperand` authority, and the compatibility `emit_rval_payload` wrapper remains string-returning | Implemented: native Add/Mul and i32 refs remain exact; generic ownership rejects invalid/duplicate result IDs and unknown/cross-function uses; invalid opcode or missing type authority also rejects | `%tN`, instruction order, rendered LLVM, and opcode/type text never establish identity; misleading result/use displays with the same ID verify successfully |

The SSA-argument probe is therefore distinct evidence but not a distinct
implementation seam: it must consume the common native argument carrier first
introduced for the immediate-argument path. CFG labels, body parameters,
stack/local objects, and alloca identity remain outside these four contracts.

## Step-3 direct integer-call result contract

The claimed row is deliberately structural and narrow: nonvoid integer
`LirCallOp` with a valid `direct_callee_link_name_id` and a structured
`callee_signature`. Its producer allocates a function-owned value before
rendering, stores that exact operand in the call, and returns the same operand
from CallExpr lowering. Void calls remain empty/no-authority. Indirect calls,
intrinsic/raw calls, non-integer results, argument carriers, and ABI expansion
remain compatibility rows.

Reachable verification requires the claimed call result to carry a valid
`LirValueId` and its present structured callee signature to carry an exact
return type ref agreeing with the call return type; the generic ownership pass
rejects invalid or duplicate definitions and unknown or cross-function
downstream uses. Focused malformed fixtures cover missing result authority,
missing/mismatched return refs, invalid, duplicate, unknown, cross-function,
and void-result cases; misleading result/use displays prove that no name
recovery participates.

## Step-4 direct void fixed immediate contract

The claimed row is a direct void call with one fixed, specified, nonvariadic
integer parameter and one integer-immediate structured argument. The producer
retains the source `LirIntegerImmediate` only when fixed-parameter coercion
keeps the LLVM representation, supplies exact argument `LirTypeRef` facts, and
keeps extension metadata `None`. Formatting consumes presentation from the
operand but cannot change its authority.

Reachable verification uses the structured row first: fixed signature and
argument type refs must agree, exactly one structured argument must carry a
representable native immediate, extension must be `None`, and the void call
must have no result. Once those facts are complete, raw `args_str`, suffix,
operand display, and raw argument/parameter type spellings are ignored.
Incomplete or unclaimed indirect, variadic, ABI-expanded, aggregate, and SSA
rows retain compatibility verification; Step 4 does not infer an immediate
from any presentation. In particular, integer-looking character-literal
compatibility text may classify as `Immediate`, but without the producer's
exact structured argument type refs it does not enter the Step-4 claim.

## Step-5 direct void fixed SSA contract

The claimed row is a direct `LinkNameId`-resolved void call with one fixed,
specified, nonvariadic integer parameter whose source expression is the
authoritative selected-global scalar load from CC-LOAD-1. The producer uses the
HIR global `DeclRef` as the structural route fact and preserves the exact
source `LirOperand{LirValueId}` through the common call-argument carrier only
when fixed-parameter coercion keeps the LLVM representation. No symbol or
temporary spelling participates.

Reachable verification requires exactly one structured SSA argument, exact
and agreeing signature/argument type refs, extension `None`, and no void-call
result. The generic ownership pass requires its valid ID to resolve to an
existing definition in the current function, rejecting unknown and
cross-function uses. Missing, wrong-alternative, invalid, type, signature,
count, and extension conflicts reject. Once native authority is complete,
`args_str`, suffix text, operand display, and raw type mirrors are ignored.
The selected-global load contract is unchanged; other SSA sources, indirect,
variadic, ABI-expanded, aggregate, and object routes remain compatibility.

## Step-6 ordinary scalar integer binary contract

The claimed row is normalized, non-pointer, non-vector integer arithmetic in
PB's ordinary `BinaryExpr` arithmetic branch. `emit_binary_rval_operand`
allocates each `LirBinOp.result` through `fresh_value` and returns that same
operand. An input keeps native authority only when arithmetic conversion and
the final coercion leave its representation unchanged. Integer immediates must
also be representable by the normalized operation width; a converted literal
whose spelling instead denotes modulo or bit-pattern semantics remains a
monostate compatibility operand. The existing string-returning payload wrapper
is retained for callers that do not consume native authority.

The focused Add-then-Mul chain therefore owns two distinct result IDs, and the
Mul lhs is the exact Add result ID. Native `LirBinaryOpcodeRef` and
`LirTypeRef` facts remain the semantic opcode/type authority. Generic
function ownership registers each modeled result once and resolves modeled
lhs/rhs uses in the current function, rejecting invalid/duplicate definitions
and unknown/cross-function uses. Invalid opcode and missing type authority also
reject through existing reachable checks, while misleading operand displays
with unchanged IDs pass.

This proves the common allocator/operand mechanism is reusable by neighboring
ordinary scalar result/use rows. Steps 7.1 through 7.5 publish the
representative explicit scalar integer cast, ordinary scalar integer compare,
current i32 scalar select, current integer abs route, and representative
ordinary scalar floating binary chain. Aggregate/vector rows need their own
type/index/mask semantics; CFG, parameters, pointer/object, inline-asm, other
call families, and BIR remain distinct or outside this idea.

## Step-7.1 explicit scalar integer cast contract

The claimed row is an explicit, non-pointer, non-vector scalar integer
`CastExpr` whose source and destination widths differ. `emit_cast_rval_operand`
obtains the source through the common operand path and calls the narrow
`coerce_operand` seam. That seam allocates the `LirCastOp.result` through
`fresh_value`, preserves the source operand, stores native `LirCastKind` plus
exact from/to `LirTypeRef` facts, and returns the same result operand. The
existing string `coerce` path remains unchanged for all other cast producers.

The focused i32-to-i64 SExt result feeds one later i64 Add without another
representation change, so the Add lhs carries the exact cast result ID.
Generic function ownership registers the cast result and resolves the later
use in the current function, rejecting invalid/duplicate results and
unknown/cross-function uses. Reachable cast verification rejects invalid enum
values, missing integer endpoint authority, Trunc without narrowing, extension
without widening, and authoritative non-integer cast alternatives. Misleading
cast-result/use displays with unchanged IDs pass.

This packet does not claim same-width no-op coercions because they emit no
`LirCastOp`. Steps 7.7 through 7.12 separately own authoritative-source
explicit FPTrunc, FPExt, SIToFP, UIToFP, FPToSI, and FPToUI. Other floating,
pointer, bitcast, vector, aggregate, implicit-coercion, and other explicit cast
shapes remain compatibility rows.
Steps 7.2 through 7.4 separately own the representative scalar integer compare,
current select, and integer abs route; CFG/parameters, object identity, other
calls, inline assembly, and BIR are unchanged.

## Step-7.2 ordinary scalar integer compare contract

The claimed row is a non-pointer, non-vector ordinary scalar integer comparison
in PB's `BinaryExpr` comparison branch. It allocates `LirCmpOp.result` through
`fresh_value`, preserves unchanged source operands through the common
`LirOperand` carrier, and stores the native integer `LirCmpPredicateRef` with
the exact compared `LirTypeRef`.

Current lowering immediately normalizes the i1 comparison result to i32. That
directly coupled `LirCastOp` remains a monostate-result compatibility producer,
but its operand is the exact authoritative compare result ID. Generic function
ownership therefore registers the compare result and resolves the cast use in
the same function, rejecting invalid/duplicate results and unknown or
cross-function uses. Reachable comparison verification rejects invalid or
non-integer predicates, missing type authority, and float/type conflicts on an
authoritative integer result. Misleading compare-result/cast-operand displays
with unchanged IDs pass.

This packet does not reopen the Step-7.1 cast producer claim: the normalization
cast has no authoritative result. Step 7.6 separately owns the PB ordinary
scalar floating comparison. Pointer, vector, complex/logical-helper, builtin,
vaarg, and statement comparison producers remain compatibility. Steps 7.3 and
7.4 separately own the current scalar select and integer abs route;
pointer/object, aggregate/vector, CFG/parameters, other calls, inline assembly,
and BIR are unchanged.

## Step-7.3 current scalar select contract

The only active `LirSelectOp` producer is the i32 `__builtin_ffs` route.
`emit_builtin_ffs_call` allocates its select result through `fresh_value`,
stores exact integer `LirTypeRef` authority, publishes the structurally known
zero arm as `LirIntegerImmediate{0}`, and returns the same result operand through
the builtin CallExpr path. Step 7.15 owns the condition's equality-to-zero
source, while Step 7.14 separately owns the false arm's add-one source.

The focused ffs result feeds one later ordinary i32 Add without a representation
change, so the Add lhs carries the exact select result ID. Generic function
ownership rejects invalid/duplicate results and unknown/cross-function uses.
Reachable select verification requires an authoritative result for the integer
route, rejects authoritative non-integer type conflicts, and requires the
condition to retain SSA shape and resolve to its current-function comparison;
missing or wrong-alternative condition authority rejects. Misleading
select-result/use displays with unchanged IDs pass.

Step 7.13 separately owns the wider ffs select-to-Trunc chain, Step 7.14 owns
the shared add-one-to-false-arm edge, Step 7.15 owns the zero-comparison-to-
condition edge, and Step 7.16 owns cttz-to-add-one. Step 7.4 separately owns
the current integer abs route;
aggregate/vector, pointer/object, CFG/parameters, other calls, inline assembly,
and BIR are unchanged.

## Step-7.4 current integer abs contract

The active `LirAbsOp` producer is the existing integer `abs`/`labs`/`llabs`
branch in `emit_post_builtin_call_operand`. It now obtains the argument through
the common `emit_rval_operand` carrier, applies the existing scalar-integer
`coerce_operand` contract, allocates the abs result through `fresh_value`, and
stores exact i32 or i64 `LirTypeRef` authority. Argument authority survives
only when structurally available: the focused selected-global load keeps its
exact current-function ID and the immediate neighbor keeps its native integer
payload; monostate SSA remains valid for sources without native identity.

The focused i32 abs result feeds one later ordinary Add without a representation
change, so the Add lhs carries the exact abs result ID. Generic function
ownership registers the result, resolves native argument/result uses in the
current function, and rejects invalid/duplicate results plus unknown or
cross-function uses. Reachable abs verification requires a native result for
the integer route, rejects authoritative noninteger or missing type conflicts,
and confines the argument to SSA/immediate shape. Missing arguments and wrong
global-authority alternatives reject, while misleading argument/result/use
displays with unchanged authority pass.

This packet does not broaden call publication: it changes only the existing
integer abs special branch. Other builtin/direct/indirect calls, noninteger
operations, aggregate/vector, pointer/object, CFG/parameters, inline assembly,
and BIR remain unchanged.

## Step-7.5 ordinary scalar floating binary contract

The claimed row is ordinary, nonpointer, nonvector scalar floating arithmetic
in PB's existing `BinaryExpr` arithmetic table, after complex, vector, pointer,
and logical-helper routes have exited. It reuses the Step-6 common carrier:
each focused `LirBinOp.result` is allocated through `fresh_value`, the native
floating `LirBinaryOpcodeRef` and exact `LirTypeRef` are retained, and the same
result operand returns through `emit_binary_rval_operand`. Initial floating
literals have no native payload carrier and therefore remain honest monostate
operands.

The focused double FAdd-then-FMul chain owns two distinct results, and the FMul
lhs carries the exact FAdd result ID. Generic function ownership rejects
invalid/duplicate results and unknown/cross-function uses. Reachable binary
verification rejects invalid opcodes and missing types; for an authoritative
result it also requires floating opcode and floating type alternatives to
agree, rejecting integer-opcode/floating-type and floating-opcode/integer-type
conflicts. Misleading result/use displays with unchanged IDs pass.

This packet does not publish floating literal authority or broaden complex,
vector, pointer, logical-helper, compound-assignment, builtin, vaarg, or
statement binary producers. Previously closed cast/integer-compare/select/abs/
call families remain unchanged; Step 7.6 separately owns the PB ordinary
floating comparison. CFG/parameters, inline assembly, and BIR remain unchanged.

## Step-7.6 ordinary scalar floating compare contract

The claimed row is PB's ordinary, nonpointer, nonvector scalar floating
comparison after complex, vector, pointer, and logical-helper routes have
exited. The existing comparison table now allocates this result through
`fresh_value`, stores native floating mode, native `LirCmpPredicateRef`, and the
exact compared `LirTypeRef`, then passes that same result operand to the
existing i1-to-i32 normalization cast. Floating literals have no native payload
carrier and remain honest monostate operands.

The focused double OLt result has one valid function-owned ID, and the
compatibility-result ZExt consumes that exact ID. Generic function ownership
rejects invalid/duplicate results and unknown/cross-function uses. Reachable
comparison verification requires authoritative mode, predicate family, and
type family to agree: invalid predicates, integer predicates in floating mode,
missing/integer compared types, and a false floating-mode claim reject.
Misleading comparison-result/cast-operand displays with unchanged IDs pass.

This packet does not publish the normalization cast result or broaden pointer,
vector, complex, logical-helper, builtin, vaarg, statement, aggregate/object,
CFG/parameter, call, inline-assembly, or BIR routes. All other Step-7 rows and
previously closed producer families remain unchanged.

## Step-7.7 explicit scalar FPTrunc contract

The claimed row is one explicit, nonpointer, nonvector scalar floating
`CastExpr` that narrows an authoritative Step-7.5 result. The focused double
FAdd first allocates a native result ID; `emit_cast_rval_operand` passes that
same operand to `coerce_operand`, which enters the native floating branch only
when the source has a `LirValueId` and the destination representation is
narrower. It allocates the `LirCastOp.result` through `fresh_value`, stores
native FPTrunc with exact double-to-float endpoints, and returns the same result
operand to a later float FMul.

Generic ownership resolves both the FAdd-to-FPTrunc source edge and the
FPTrunc-to-FMul result edge, rejecting invalid/duplicate results and unknown or
cross-function uses. Reachable cast verification requires FPTrunc, exact
floating endpoints, and strict narrowing direction; wrong kind, missing or
nonfloating endpoints, and equal/widening direction reject. Misleading
source/result/use displays with unchanged IDs pass.

Step 7.8 separately owns authoritative-source FPExt. Integer/floating
conversions, pointer/bitcast/vector/complex/aggregate casts, implicit
coercions, monostate-source floating casts, and all other producer families
remain compatibility or outside this packet.

## Step-7.8 explicit scalar FPExt contract

The claimed row is one explicit, nonpointer, nonvector scalar floating
`CastExpr` that widens an authoritative Step-7.5 result. The focused float FAdd
first allocates a native result ID; `emit_cast_rval_operand` passes that exact
operand to `coerce_operand`, whose native floating branch requires a source
`LirValueId` and a real representation-width change. It allocates the
`LirCastOp.result` through `fresh_value`, stores native FPExt with exact
float-to-double endpoints, and returns the identical result operand to a later
double FMul.

Generic ownership resolves both the FAdd-to-FPExt source edge and the
FPExt-to-FMul result edge, rejecting invalid/duplicate results and unknown or
cross-function uses. Reachable cast verification requires FPExt, exact floating
endpoints, and strict widening direction; wrong kind, missing or nonfloating
endpoints, and equal/narrowing direction reject. Misleading source/result/use
displays with unchanged IDs pass.

Step 7.9 separately owns authoritative-source SIToFP. UIToFP, reverse
integer/floating conversions, pointer/bitcast/vector/complex/aggregate casts,
implicit coercions, same-representation no-ops, monostate-source casts, and all
other producer families remain compatibility or outside this packet.

## Step-7.9 explicit scalar SIToFP contract

The claimed row is one explicit, nonpointer, nonvector conversion from a signed
scalar integer to a scalar floating type. The focused Step-6 i32 Add first
allocates a native result ID; `emit_cast_rval_operand` passes that exact operand
to `coerce_operand`, whose native SIToFP branch requires an authoritative
signed integer source and exact scalar integer-to-floating endpoint families.
It allocates the `LirCastOp.result` through `fresh_value`, stores native SIToFP
with exact i32-to-double endpoints, and returns the identical result operand to
a later double FMul.

Generic ownership resolves both the Add-to-SIToFP source edge and the
SIToFP-to-FMul result edge, rejecting invalid/duplicate results and unknown or
cross-function uses. Reachable cast verification requires native SIToFP, an
exact integer source, and an exact floating destination; wrong kind, missing
endpoints, floating sources, and integer destinations reject. Misleading
source/result/use displays with unchanged IDs pass.

Step 7.10 separately owns authoritative-source UIToFP. FPToSI, FPToUI,
pointer/bitcast/vector/complex/aggregate casts, implicit coercions,
monostate-source conversions, and all other producer families remain
compatibility or outside this packet.

## Step-7.10 explicit scalar UIToFP contract

The claimed row is one explicit, nonpointer, nonvector conversion from an
unsigned scalar integer to a scalar floating type. The focused Step-6 unsigned
i32 Add first allocates a native result ID; `emit_cast_rval_operand` passes that
exact operand to `coerce_operand`, whose native integer-to-floating branch
selects UIToFP from the source `TypeSpec` and requires exact scalar endpoint
families. It allocates the `LirCastOp.result` through `fresh_value`, stores
native UIToFP with exact i32-to-double endpoints, and returns the identical
result operand to a later double FMul.

Generic ownership resolves both the Add-to-UIToFP source edge and the
UIToFP-to-FMul result edge, rejecting invalid/duplicate results and unknown or
cross-function uses. Reachable cast verification requires an integer-to-
floating native kind, exact integer source, and exact floating destination;
reverse kind, missing endpoints, floating sources, and integer destinations
reject. Production chooses UIToFP from unsigned source semantics, while native
LIR integer type refs remain signless and therefore do not independently
distinguish an SIToFP/UIToFP kind swap. Misleading source/result/use displays
with unchanged IDs pass.

Step 7.11 separately owns authoritative-source FPToSI. FPToUI,
pointer/bitcast/vector/complex/aggregate casts, implicit coercions,
monostate-source conversions, and all other producer families remain
compatibility or outside this packet.

## Step-7.11 explicit scalar FPToSI contract

The claimed row is one explicit, nonpointer, nonvector conversion from a scalar
floating type to a signed scalar integer. The focused Step-7.5 double FAdd
first allocates a native result ID; `emit_cast_rval_operand` passes that exact
operand to `coerce_operand`, whose native FPToSI branch requires an
authoritative exact floating source and a signed scalar integer destination.
It allocates the `LirCastOp.result` through `fresh_value`, stores native FPToSI
with exact double-to-i32 endpoints, and returns the identical result operand to
a later i32 Add.

Generic ownership resolves both the FAdd-to-FPToSI source edge and the
FPToSI-to-Add result edge, rejecting invalid/duplicate results and unknown or
cross-function uses. Reachable cast verification requires native FPToSI, an
exact floating source, and an exact integer destination; reverse kind, missing
endpoints, integer sources, and floating destinations reject. Production
chooses FPToSI from signed destination semantics, while native LIR integer type
refs remain signless. Misleading source/result/use displays with unchanged IDs
pass.

Step 7.12 separately owns authoritative-source FPToUI.
Pointer/bitcast/vector/complex/aggregate casts, implicit coercions,
monostate-source conversions, and all other producer families remain
compatibility or outside this packet.

## Step-7.12 explicit scalar FPToUI contract

The claimed row is one explicit, nonpointer, nonvector conversion from a scalar
floating type to an unsigned scalar integer. The focused Step-7.5 double FAdd
first allocates a native result ID; `emit_cast_rval_operand` passes that exact
operand to `coerce_operand`, whose native floating-to-integer branch selects
FPToUI from the unsigned destination `TypeSpec` and requires exact scalar
endpoint families. It allocates the `LirCastOp.result` through `fresh_value`,
stores native FPToUI with exact double-to-i32 endpoints, and returns the
identical result operand to a later unsigned i32 Add.

Generic ownership resolves both the FAdd-to-FPToUI source edge and the
FPToUI-to-Add result edge, rejecting invalid/duplicate results and unknown or
cross-function uses. Reachable cast verification requires a native floating-
to-integer kind, exact floating source, and exact integer destination; reverse
kind, missing endpoints, integer sources, and floating destinations reject.
Production chooses FPToUI from unsigned destination semantics, while native LIR
integer type refs remain signless and therefore do not independently
distinguish an FPToSI/FPToUI kind swap. Misleading source/result/use displays
with unchanged IDs pass.

Pointer/bitcast/vector/complex/aggregate casts, implicit coercions,
monostate-source conversions, and all other producer families remain
compatibility or outside this packet.

## Step-7.13 wide builtin-ffs select narrowing contract

The claimed row is only PI's i64 `__builtin_ffsll` select result and its
required i32 return narrowing. `emit_builtin_ffs_call` allocates the exact i64
`LirSelectOp.result` through `fresh_value`, then passes that same operand to the
existing integer `coerce_operand` seam. The seam allocates a second exact
result through `fresh_value`, stores native Trunc with exact i64-to-i32
endpoints, and returns the identical cast result operand through builtin
CallExpr lowering to a later ordinary i32 Add.

Generic ownership resolves both the select-to-Trunc and Trunc-to-Add edges,
rejecting invalid/duplicate definitions and unknown or cross-function uses.
Reachable cast verification requires native Trunc, exact integer endpoints,
and strict narrowing direction; wrong kind, missing or noninteger endpoints,
and equal/widening direction reject. Misleading select, cast, and later-use
displays with unchanged IDs pass.

Step 7.14 separately owns the shared plus-one result and false arm, Step 7.15
owns the shared zero comparison and select condition, and Step 7.16 owns the
cttz call and plus-one lhs. Other builtins and calls,
other select or cast producers, pointer/vector/complex/aggregate/object work,
implicit coercions, CFG/parameters, inline assembly, and BIR remain outside
this packet.

## Step-7.14 builtin-ffs add-one/select-arm contract

The claimed row is only PI's shared scalar i32/i64 add-one operation inside
`emit_builtin_ffs_call`. The producer allocates its `LirBinOp.result` through
`fresh_value`, records native integer Add and the exact i32 or i64 type, and
publishes the structurally known rhs as `LirIntegerImmediate{1}`. Step 7.16
separately owns the exact cttz lhs. The existing
`LirSelectOp.false_val` receives that exact add-one result ID.

Generic ownership rejects invalid or duplicate add-one results and unknown or
cross-function false-arm uses. Reachable binary verification requires coherent
native integer opcode/type authority; when integer operands carry authority,
only SSA IDs or integer immediates representable by the exact operation width
are accepted. Invalid or conflicting opcode/type authority, wrong operand
authority alternatives, and unrepresentable immediates reject. Production
asserts exact immediate one, exact i32/i64 width, and exact add-one-to-false-arm
identity. Misleading result and false-arm displays with unchanged IDs pass.
The common ordinary scalar publication seam withholds native authority from
converted integer literals that are not representable by the normalized
operation width, so the generic verifier invariant does not reinterpret their
legacy display payload. This does not affect the exact representable ffs
immediate one.

Step 7.15 separately owns the zero comparison and select condition, and Step
7.16 owns the cttz result and plus-one lhs. Other builtins, calls,
binary/select producers,
pointer/vector/complex/aggregate/object work, CFG/parameters, inline assembly,
and BIR remain outside this packet.

## Step-7.15 builtin-ffs zero-comparison/select-condition contract

The claimed row is only PI's shared scalar i32/i64 equality-to-zero operation
inside `emit_builtin_ffs_call`. The producer allocates its `LirCmpOp.result`
through `fresh_value`, records integer mode, native Eq, and the exact i32 or i64
compared type, retains the prepared argument as honest monostate SSA or
immediate presentation when no native authority is available, and publishes
`LirIntegerImmediate{0}`. The
existing `LirSelectOp.cond` receives that exact comparison result ID.

Generic ownership rejects invalid or duplicate comparison results and unknown
or cross-function condition uses. The authoritative integer select condition
must resolve to its current-function comparison definition; when that
comparison's operands carry authority, only SSA IDs or integer immediates
representable by the exact compared width are accepted. Invalid or conflicting
predicate, mode, or type authority, wrong operand authority alternatives, and
unrepresentable immediates reject. Production asserts exact Eq, i32/i64 width,
zero, and comparison-to-condition identity. Misleading comparison-result and
condition displays with unchanged IDs pass.

Step 7.16 separately owns the cttz result and plus-one lhs. Other builtin
comparisons, calls, selects, pointer/vector/complex/aggregate/object work,
CFG/parameters, inline assembly, and BIR remain outside this packet.

## Step-7.16 builtin-ffs cttz-call/add-lhs contract

The claimed row is only PI's i32/i64 `llvm.cttz` call inside
`emit_builtin_ffs_call`. Its result is allocated through `fresh_value` and the
exact result ID is the accepted Step-7.14 add-one lhs. The callee carries a
module-owned `LinkNameId`, and `LirIntrinsicKind::Cttz` records the semantic
intrinsic category without relying on its spelling. The nonvariadic signature
has exact matching integer return/first-parameter refs and an i1 second
parameter. Structured arguments retain the prepared value as honest monostate
SSA or literal Immediate presentation when unavailable and publish the
structural false flag as `LirIntegerImmediate{0}` with exact i1 type.
`LirZeroCountBehavior::Defined` records why that false flag is required without
creating a second intrinsic kind.

The authority-first verifier recognizes the native Cttz kind and fixed
integer/i1 boolean-flag shape rather than an intrinsic spelling. It requires a
resolvable callee ID shared by the operand and direct-callee field, complete
matching signature, return and argument refs, exact counts, no variadic/
unspecified/ABI extension facts, valid SSA/immediate prepared-value
presentation, and immediate false. Any native prepared-value authority must be
a valid current-function SSA ID; literal presentation remains monostate.
Generic ownership
rejects invalid/duplicate call results and unknown or cross-function add-one
uses. Missing, wrong-alternative, conflicting, or unresolved callee,
signature, type, argument, flag, result, and use authority reject. Misleading
callee/call/result/add displays with unchanged native authority pass.

Builtin-ctz Cttz is owned separately by Step 7.17. Other ctlz/ctpop and
intrinsic/builtin/direct/indirect calls, ABI and
variadic work, pointer/vector/complex/aggregate/object families,
CFG/parameters, inline assembly, and BIR remain outside this packet.

## Step-7.17 builtin-ctz call/narrow/final-use contract

The claimed row is only PI's i32/i64 `emit_builtin_ctz_call` route. It reuses
the native `LirIntrinsicKind::Cttz`, module-owned intrinsic `LinkNameId`, and
exact fixed nonvariadic integer/i1 signature from Step 7.16. A separate native
`LirZeroCountBehavior::Undefined` fact requires the second argument to be exact
i1 `LirIntegerImmediate{1}`; it distinguishes ctz from ffs structurally without
matching callee spelling or inventing another intrinsic kind. Arg0 retains the
same honest monostate SSA or Immediate presentation compatibility boundary.

The call result is allocated through `fresh_value`. The i32 route returns that
exact operand to a later ordinary i32 Add. The i64 route allocates a fresh
authoritative `LirCastOp` Trunc, preserves the exact call result as its source,
requires exact i64-to-i32 endpoints, and returns the exact trunc result to the
later Add. Reachable verification rejects missing or conflicting zero behavior,
false or wrong-alternative flag authority, malformed call results, unknown
call-to-trunc or final-use IDs, wrong cast kind/endpoints, and payload authority
invented for compatible literal input. Misleading call/callee/argument/trunc/
use displays pass when all native IDs and semantic facts remain unchanged.

Step 7.16's ffs false/defined-zero contract remains exact. Clz is owned by
Step 7.18. Popcount,
parity, all other call/narrowing producers, ABI/variadic work, pointer/vector/
aggregate/object families, CFG/parameters, inline assembly, and BIR remain
outside this packet.

## Step-7.18 builtin-clz call/narrow/final-use contract

The claimed row is only PI's i32/i64 `emit_builtin_clz_call` route. It publishes
native `LirIntrinsicKind::Ctlz`, a module-owned intrinsic `LinkNameId`, and an
exact fixed nonvariadic integer/i1 signature. The shared native
`LirZeroCountBehavior::Undefined` fact requires exact i1
`LirIntegerImmediate{1}` without matching callee spelling. Arg0 retains the
same honest monostate SSA or Immediate presentation compatibility boundary as
the accepted Cttz routes.

The call result is allocated through `fresh_value`. The i32 route returns that
exact operand to a later ordinary i32 Add. The i64 route allocates a fresh
authoritative `LirCastOp` Trunc, preserves the exact call result as its source,
requires exact i64-to-i32 endpoints, and returns the exact trunc result to the
later Add. Reachable verification rejects missing native kind or zero behavior,
conflicting behavior/flag authority, malformed results, unknown call-to-trunc
or final-use IDs, wrong cast kind/endpoints, and invented literal payload
authority. Misleading call/callee/argument/trunc/use displays pass when native
IDs and semantic facts remain unchanged.

Steps 7.16/7.17 retain their exact Cttz contracts. Popcount is owned by Step
7.19. Parity and all other
call/narrowing producers, ABI/variadic work, pointer/vector/aggregate/object
families, CFG/parameters, inline assembly, and BIR remain outside this packet.

## Step-7.19 builtin-popcount call/narrow/final-use contract

The claimed row is only PI's i32/i64 `emit_builtin_popcount_call` route. It
publishes native `LirIntrinsicKind::Ctpop`, a module-owned intrinsic
`LinkNameId`, and an exact fixed nonvariadic one-integer-parameter signature.
Unlike Cttz/Ctlz, Ctpop must carry no `zero_count_behavior`. Arg0 retains the
same honest monostate SSA or Immediate presentation compatibility boundary.

The call result is allocated through `fresh_value`. The i32 route returns that
exact operand to a later ordinary i32 Add. The i64 route allocates a fresh
authoritative `LirCastOp` Trunc, preserves the exact call result as its source,
requires exact i64-to-i32 endpoints, and returns the exact trunc result to the
later Add. Reachable verification rejects wrong intrinsic kind, any zero-count
behavior, malformed signature/count/result authority, unknown call-to-trunc or
final-use IDs, wrong cast kind/endpoints, and invented literal payload
authority. Misleading call/callee/argument/trunc/use displays pass when native
IDs and semantic facts remain unchanged.

Steps 7.16-7.18 retain their exact Cttz/Ctlz contracts. Parity and all other
call/narrowing producers, ABI/variadic work, pointer/vector/aggregate/object
families, CFG/parameters, inline assembly, and BIR remain outside this packet.

## Step-7.21 scalar inline-asm output binding contract

The claimed row is only PS's scalar integer output-only `LirInlineAsmOp`
binding. The producer allocates the semantic output through `fresh_value`,
stores that exact operand in `ordinary_results[0]` with native integer type,
`Output` role, and output constraint index zero, and passes the same operand
through representation-preserving coercion to the existing later Store. The
compatibility `result` retains only the same display and no authority.

Reachable verification requires the nonvoid scalar integer operation to carry
one exact semantic result binding with matching return type and index. It
collects authoritative semantic output bindings as function-owned definitions,
rejects invalid/duplicate/missing/wrong-role/type/index bindings and unknown or
cross-function uses, and requires a Store consuming that ID to use the binding
type. Misleading compatibility-result, binding, Store, rendered operand,
assembly, and constraint displays pass when native facts and IDs are unchanged.

Input, tied/read-write, memory, address, immediate, clobber, explicit-register,
vector, and multi-output bindings, `insn_r` semantics, opaque text,
compatibility-result authority, stack/local/body-parameter publication, CFG,
BIR receipt, and idea-741 contracts remain outside this packet.

## Step-7.25 i64 inline-asm output binding contract

The claimed row is only PS's single non-explicit-register output-only scalar
i64 `LirInlineAsmOp` binding. It needs no producer or schema specialization:
the Step-7.21 generic path resolves the output as i64, allocates its semantic
result through `fresh_value`, stores that exact operand in the sole native
i64/`Output`/constraint-index-zero result binding, and preserves the same ID
through representation-preserving coercion into the later i64 Store. The
compatibility result and every rendered or opaque string remain authority-free.

Focused i64 coverage proves the exact definition/binding/Store chain and
display independence. Reachable verification rejects invalid, duplicate,
missing, or wrong-alternative definitions; wrong role, index, count, position,
binding width, or operation width; unknown or cross-function Store uses; and
binding-to-Store width conflicts. The accepted Step-7.21 i32 behavior is the
nearby same-mechanism regression neighbor.

Inputs, tied/read-write, multi-output, memory, address, immediate, clobber,
explicit-register, floating/vector/aggregate bindings, `insn_r` semantics,
opaque-text interpretation, compatibility-result authority, stack/local/
object/body-parameter publication, CFG, BIR receipt, parity, and idea-741
contracts remain outside this packet.

## Step-7.32 direct external double call-result contract

The claimed row is the normal resolved `hir::Function` route for a block-scope
`extern double target(void)` declaration. The bodyless extern Function lowers
as the single fixed-void LIR declaration, and `emit_call_with_result` uses the
existing generic resolved-direct-call seam: `fresh_value(ctx)` allocates the
caller-owned result before construction, while the same `LirOperand` becomes
the direct `LirCallOp.result` and the following double `FAdd` lhs. The call and
declaration share one `LinkNameId`; their native floating return and fixed-void
signatures are structured facts, not recovered from names or rendered text.

The focused `test_block_scope_extern_void_prototype_uses_direct_function_entity`
fixture proves the positive declaration, direct-call, result-to-`FAdd` chain.
Its reachable malformed variants reject a missing matching declaration,
declaration or call-signature conflicts, missing result authority, duplicate
result IDs, unknown downstream IDs, and the `FAdd` type conflict. The verifier
requires exactly one module Function with the direct `LinkNameId` and a
matching fixed-void structured signature, then generic function ownership
checks the result/use ID edge. This disposition covers only resolved,
zero-argument native floating direct Function calls; indirect, variadic,
argument-bearing, aggregate/object, ABI, and BIR routes remain outside this
packet.

## Mechanical coverage check

Reproducible source-side extraction:

```sh
sed -n '/using LirInst = std::variant</,/^>;/p' src/codegen/lir/ir.hpp \
  | rg -o '^    Lir[A-Za-z0-9]+' | sed 's/^    //' | sort -u
```

Document-side extraction:

```sh
sed -n '/## Mechanical 38-alternative matrix/,/## Separate adjacent/p' \
  docs/lir_remaining_ordinary_value_identity/authority_matrix.md \
  | rg -o '^\| `Lir[A-Za-z0-9]+' | sed 's/^| `//' \
  | rg -v '^LirInst$' | sort -u
```

`comm -3` between those outputs is empty. Both counts are **38**, all names are
unique, and the production scan finds **12 producerless legacy alternatives +
26 active modern alternatives**. There is no catch-all row.

Current-source spot checks used for this baseline:

- direct structured integer call result: `call/target.cpp` uses `fresh_value`,
  the operand-taking call maker retains it, and `emit_rval_call_operand` keeps
  the same carrier through CallExpr lowering; focused immediate arguments now
  use `emit_rval_operand`, the `OwnedLirTypedCallArg` native operand carrier,
  and authority-preserving structured construction. The focused
  selected-global SSA argument reuses that carrier and the exact CC-LOAD-1
  result ID; other argument rows fall back to monostate compatibility without
  parsing presentation;
- representative scalar chain: `expr/binary.cpp` constructs `LirBinOp` from
  `fresh_value` results and authority-preserving unchanged operands for the
  normalized scalar integer branch while carrying native
  `LirBinaryOpcodeRef`/`LirTypeRef`; other binary branches remain compatibility;
- representative scalar floating chain: the same ordinary arithmetic table
  allocates double FAdd/FMul results through `fresh_value`, preserves the exact
  first result ID into the later use, and keeps unavailable literal authority
  monostate while complex/vector/pointer/logical routes remain compatibility;
- representative scalar cast: `expr/misc.cpp` routes explicit `CastExpr`
  through `coerce_operand`; width-changing scalar integer casts allocate one
  authoritative result and preserve exact native kind/from/to authority, while
  other `coerce` producers remain compatibility;
- representative scalar FPTrunc: the explicit cast seam accepts only an
  authoritative Step-7.5 floating source with narrowing representation,
  preserves that exact source ID, allocates exact double-to-float FPTrunc
  authority, and passes the cast result ID to a later float FMul;
- representative scalar FPExt: the same explicit cast seam accepts an
  authoritative Step-7.5 floating source with widening representation,
  preserves that exact source ID, allocates exact float-to-double FPExt
  authority, and passes the cast result ID to a later double FMul;
- representative scalar SIToFP: the explicit cast seam accepts an authoritative
  Step-6 signed integer result, preserves that exact source ID, allocates exact
  i32-to-double SIToFP authority, and passes the cast result ID to a later
  double FMul;
- representative scalar UIToFP: the same explicit cast seam accepts an
  authoritative Step-6 unsigned integer result, preserves that exact source ID,
  allocates exact i32-to-double UIToFP authority, and passes the cast result ID
  to a later double FMul;
- representative scalar FPToSI: the explicit cast seam accepts an authoritative
  Step-7.5 floating result, preserves that exact source ID, allocates exact
  double-to-i32 FPToSI authority, and passes the cast result ID to a later i32
  Add;
- representative scalar FPToUI: the same explicit cast seam accepts an
  authoritative Step-7.5 floating result with an unsigned integer destination,
  preserves that exact source ID, allocates exact double-to-i32 FPToUI
  authority, and passes the cast result ID to a later unsigned i32 Add;
- representative scalar compare: `expr/binary.cpp` allocates the ordinary
  integer comparison through `fresh_value`, retains native integer predicate
  and compared type authority, and passes the exact result ID to its existing
  monostate-result normalization cast;
- representative scalar floating compare: the same PB comparison table
  allocates double OLt through `fresh_value`, retains native floating mode,
  predicate, and exact compared type, and passes the exact result ID to the
  compatibility-result normalization cast while literals remain monostate;
- representative scalar select: `call/builtin.cpp` allocates the i32 ffs select
  through `fresh_value`, carries exact scalar type plus native zero authority,
  returns the exact result ID to a later ordinary use, and carries the exact
  Step-7.14 add-one result ID as its false arm plus the exact Step-7.15
  equality-to-zero result ID as its condition; Step 7.16 supplies the add-one's
  exact structured cttz call-result lhs;
- representative wide scalar select narrowing: the i64 ffs select allocates an
  exact result, preserves that ID into an exact fresh i64-to-i32 Trunc, and
  preserves the cast result ID into a later i32 Add while its exact native i64
  cttz, add-one, and equality-to-zero results feed the add lhs, false arm, and
  condition respectively;
- representative builtin ctz narrowing: the i32 route carries its exact native
  Cttz call result directly into a later i32 Add, while the i64 route carries
  the exact call result through a fresh exact i64-to-i32 Trunc and the exact
  trunc result into the Add; both widths publish native undefined-zero behavior
  and exact i1 true without relying on displays;
- representative builtin clz narrowing: the i32 route carries its exact native
  Ctlz call result directly into a later i32 Add, while the i64 route carries
  the exact call result through a fresh exact i64-to-i32 Trunc and the exact
  trunc result into the Add; both widths publish native undefined-zero behavior
  and exact i1 true without relying on displays;
- representative builtin popcount narrowing: the i32 route carries its exact
  native Ctpop call result directly into a later i32 Add, while the i64 route
  carries the exact call result through a fresh exact i64-to-i32 Trunc and the
  exact trunc result into the Add; both widths publish an exact one-integer-
  parameter signature and no zero-count behavior;
- representative scalar inline-asm output: PS allocates the output-only i32
  and i64 semantic bindings through the same `fresh_value` path, keeps the
  compatibility result and opaque/rendered text authority-free, and preserves
  each exact output ID into its later type-matched Store;
- representative scalar abs: `call/builtin.cpp` routes the existing integer
  abs argument through the common operand/coercion seam, allocates its exact
  i32/i64 result through `fresh_value`, and preserves the result ID into a later
  ordinary use while unavailable source authority remains compatibility;
- 741 neighbors: `expr/coordinator.cpp:474,497` populate selected-global
  `LirGepOp`/`LirLoadOp`; Steps 3 and 6 add the direct integer-call and ordinary
  scalar integer binary result sites;
  `lvalue.cpp:399` retains the authoritative store RHS operand, and
  `stmt.cpp:381` retains the authoritative return operand.
