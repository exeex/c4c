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
Step-6 ordinary scalar integer arithmetic branch in
`emit_binary_rval_operand`, plus the Step-7.1 explicit scalar integer cast in
`emit_cast_rval_operand` and the Step-7.2 ordinary scalar integer compare branch
in `emit_binary_rval_operand`, plus the Step-7.3 scalar builtin-ffs select in
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
| `LirCastOp`: `result,operand`; `kind,from_type,to_type` | Active in PX/PB/PL/PC/PI/PV/PO and PF fixed-vector parameter setup; Step-7.1 representative is PX explicit scalar integer `CastExpr` | Step-7.1 explicit width-changing scalar integer cast uses `fresh_value`, exact input `LirOperand`, and an operand-returning wrapper; other cast producers remain text-only with `fresh_tmp` | Native cast enum and exact from/to refs; authoritative integer results require coherent Trunc or ZExt/SExt widths; generic ownership rejects invalid/duplicate definitions and unknown/cross-function uses | `lir_scalar_cast_result_use_identity.c` closes only the representative explicit integer cast/use row. Float, pointer, bitcast, vector, aggregate, and other implicit/coercion producers remain separately unclaimed |
| `LirGepOp`: `result,ptr,indices`; `element_type,inbounds` | Active in PR/PL/PX/PC/PV and PF parameter setup; CC-GEP-1 exact producer is PR selected-global array branch | CC-GEP-1 uses `fresh_value`, global `LinkNameId`, typed native indices; other routes use `fresh_tmp`, raw base, often raw `LirGepIndex` presentation | Native type/bool; ownership-ready; **741 exact** only for selected-global typed path | Preserve CC-GEP-1 as **regression neighbor**. Other paths require object/base and typed-index work; `lir_local_gep_identity.c`; **separate pointer/object family** |
| `LirCallOp`: `result`; `callee,direct_callee_link_name_id`; `structured_args[].operand`; typed signature/type/ext/ABI fields; text mirrors | Active, PC: `prepare_call_args`, `emit_void_call`, `emit_call_with_result`, `make_lir_call_op_with_return_type_ref` | Structured direct integer result uses `fresh_value`; the common `OwnedLirTypedCallArg` stores `LirOperand`. Focused direct void fixed integer arguments preserve either the native immediate or the exact CC-LOAD-1 selected-global result ID, with exact type refs, when coercion keeps the representation; other arguments remain monostate compatibility | Direct integer result has exact ID ownership. The focused direct void fixed immediate and selected-global SSA rows have authority-first exact type/count/ext/result verification; SSA uses additionally resolve through current-function ownership | Steps 3-5 close scalar result, fixed immediate, and selected-global SSA rows on the common carrier; indirect/variadic/ABI/aggregate rows and intrinsic results remain unclaimed; text mirrors are presentation-only |
| `LirBinOp`: `result,lhs,rhs`; `opcode,type_str` | Active, PB scalar/complex arithmetic and logical helpers; also PL compound assignment, PI builtins, PV, PS | Step-6 normalized ordinary scalar integer arithmetic uses `fresh_value` and preserves unchanged source `LirOperand` authority; complex/vector/pointer/logical and other producers remain text-only with `fresh_tmp` | Native opcode/type refs plus exact result/use ownership for the Step-6 row; generic verifier rejects invalid/duplicate definitions and unknown/cross-function uses | `lir_scalar_ordinary_value_chain_identity.c` closes the representative generic scalar seam. Neighboring scalar producers may reuse the mechanism in later bounded packets; aggregate/vector, pointer/object, and CFG rows remain distinct |
| `LirCmpOp`: `result,lhs,rhs`; `is_float,predicate,type_str` | Active, PB comparisons/logical, PI FP/builtin checks, PV, PS loop/range lowering, `core.cpp` helpers; Step-7.2 representative is PB ordinary scalar integer comparison | Step-7.2 scalar integer comparison uses `fresh_value`, preserves unchanged source operands, and feeds the exact result operand into its existing normalization cast; other comparison producers remain text-only with `fresh_tmp` | Native integer predicate and exact compared type; authoritative integer results reject float mode/type/predicate conflict, while generic ownership rejects invalid/duplicate definitions and unknown/cross-function uses | `lir_scalar_compare_result_use_identity.c` closes only the representative ordinary integer compare/use row. Float, pointer, vector, logical-helper, builtin, vaarg, and statement comparison producers remain unclaimed |
| `LirPhiOp`: `result`; `incoming[value,label]`; `type_str` | Active, PX `emit_rval_payload(TernaryExpr)`, PB `emit_logical`, PV AArch64/AMD64 joins | Result `fresh_tmp`; incoming value and predecessor are raw strings | Native result type only; result kind/nonempty incoming; incoming entries are not visited for value ownership | Result can share generic allocation, but incoming values need `LirOperand` and predecessors need `LirBlockId`; `lir_phi_identity.c`; **distinct value+CFG carrier** |
| `LirSelectOp`: `result,cond,true_val,false_val`; `type_str` | Active only in PI `emit_builtin_ffs_call` | Step-7.3 i32 ffs route uses `fresh_value`, exact scalar type, native zero immediate, and returns the same result operand; condition/false arm remain honest monostate SSA compatibility and wider ffs narrowing remains raw | Authoritative integer select requires a native result and SSA condition shape; generic ownership rejects invalid/duplicate definitions and unknown/cross-function later uses | `lir_scalar_select_result_use_identity.c` closes the current i32 scalar select/use route. Wider narrowing and unavailable internal producer authority remain explicitly unclaimed |
| `LirInsertElementOp`: `result,vec,elem,index`; `vec_type,elem_type` | Active, PB vector-scalar arithmetic branches | Text-only operands; result `fresh_tmp` | Native types; kind/type only and ownership-ready | `lir_insertelement_identity.c`; **generic result/use plus distinct vector/index semantics** |
| `LirExtractElementOp`: `result,vec,index`; `vec_type,index_type` | Active, PX `emit_rval_payload(IndexExpr)` vector branch | Text-only operands; result `fresh_tmp` | Native types; kind/type only and ownership-ready | `lir_extractelement_identity.c`; **generic result/use plus distinct vector/index semantics** |
| `LirShuffleVectorOp`: `result,vec1,vec2,mask`; `vec_type,mask_type` | Active, PB vector-scalar splat branches | Text-only operands; result `fresh_tmp` | Native types; kind/type only and ownership-ready | `lir_shufflevector_identity.c`; mask needs distinct typed constant/vector carrier; **generic result/use plus distinct vector semantics** |
| `LirVaArgOp`: `result,ap_ptr`; `type_str` | Active, PV `emit_rval_payload(VaArgExpr)` and `emit_amd64_va_arg` semantic routes | Text-only operands; result `fresh_tmp` | Native type; kind/type only and ownership-ready | `lir_vaarg_identity.c`; **generic result** but pointer/object/ABI use is **separate va-list family** |
| `LirAllocaOp`: `result,count`; `type_str,align` | Active in PF/PS/PL/PX/PC/PI/PV/`core.cpp` for VLA, locals, temporaries and ABI copies | Result and optional count text-only; result `fresh_tmp` or named stack spelling | Native type/alignment; kind/type only and ownership-ready | Requires stack/local object ownership, dynamic-count use, and hoisted/body ordering; `lir_alloca_object_identity.c`; **separate stack/local family** |
| `LirInlineAsmOp`: compatibility `result`; semantic `ordinary_inputs/results[].value`; binding type/role/index; original text/clobbers; rendered mirrors; `insn_r` | Active, PS `emit_non_control_flow_stmt(InlineAsmStmt)` | Compatibility result and semantic bindings are text-only; outputs allocated with `fresh_tmp`; assembly/constraint/clobber text remains raw by design | Native binding types/roles/indices and R metadata; verifier checks shape/order/pairing and is ownership-ready for native binding IDs | Ordinary bindings can share generic value publication, but compatibility result is presentation-only and asm text stays opaque; `lir_inline_asm_binding_identity.c`; **generic binding values + presentation/opaque payload** |

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
the final coercion leave its representation unchanged; otherwise it remains a
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
ordinary scalar result/use rows. Steps 7.1 through 7.4 publish the
representative explicit scalar integer cast, ordinary scalar integer compare,
current i32 scalar select, and current integer abs route. Aggregate/vector rows
need their own type/index/mask semantics; CFG, parameters, pointer/object,
inline-asm, other call families, and BIR remain distinct or outside this idea.

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
`LirCastOp`. Float, pointer, bitcast, vector, aggregate, implicit-coercion, and
other explicit cast shapes remain compatibility rows. Steps 7.2 through 7.4
separately own the representative scalar integer compare, current select, and
integer abs route; CFG/parameters, object identity, other calls, inline
assembly, and BIR are unchanged.

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
cast has no authoritative result. Float, pointer, vector, logical-helper,
builtin, vaarg, and statement comparison producers remain compatibility. Steps
7.3 and 7.4 separately own the current scalar select and integer abs route;
pointer/object, aggregate/vector, CFG/parameters, other calls, inline assembly,
and BIR are unchanged.

## Step-7.3 current scalar select contract

The only active `LirSelectOp` producer is the i32 `__builtin_ffs` route.
`emit_builtin_ffs_call` allocates its select result through `fresh_value`,
stores exact integer `LirTypeRef` authority, publishes the structurally known
zero arm as `LirIntegerImmediate{0}`, and returns the same result operand through
the builtin CallExpr path. The condition and false arm originate in internal
compatibility producers and therefore remain monostate SSA operands.

The focused ffs result feeds one later ordinary i32 Add without a representation
change, so the Add lhs carries the exact select result ID. Generic function
ownership rejects invalid/duplicate results and unknown/cross-function uses.
Reachable select verification requires an authoritative result for the integer
route, rejects authoritative non-integer type conflicts, and requires the
condition to retain SSA shape; missing or wrong-alternative condition authority
rejects while honest monostate SSA compatibility remains accepted. Misleading
select-result/use displays with unchanged IDs pass.

The wider ffs route still narrows through a compatibility cast and does not
publish its select result downstream. No internal cttz call, plus-one binary,
or zero-comparison result authority is inferred. Step 7.4 separately owns the
current integer abs route; aggregate/vector, pointer/object, CFG/parameters,
other calls, inline assembly, and BIR are unchanged.

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
- representative scalar cast: `expr/misc.cpp` routes explicit `CastExpr`
  through `coerce_operand`; width-changing scalar integer casts allocate one
  authoritative result and preserve exact native kind/from/to authority, while
  other `coerce` producers remain compatibility;
- representative scalar compare: `expr/binary.cpp` allocates the ordinary
  integer comparison through `fresh_value`, retains native integer predicate
  and compared type authority, and passes the exact result ID to its existing
  monostate-result normalization cast;
- representative scalar select: `call/builtin.cpp` allocates the i32 ffs select
  through `fresh_value`, carries exact scalar type plus native zero authority,
  and returns the exact result ID to a later ordinary use while internal inputs
  remain compatibility;
- representative scalar abs: `call/builtin.cpp` routes the existing integer
  abs argument through the common operand/coercion seam, allocates its exact
  i32/i64 result through `fresh_value`, and preserves the result ID into a later
  ordinary use while unavailable source authority remains compatibility;
- 741 neighbors: `expr/coordinator.cpp:474,497` populate selected-global
  `LirGepOp`/`LirLoadOp`; Steps 3 and 6 add the direct integer-call and ordinary
  scalar integer binary result sites;
  `lvalue.cpp:399` retains the authoritative store RHS operand, and
  `stmt.cpp:381` retains the authoritative return operand.
