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
Step-3 structured direct integer-call result in `emit_call_with_result`. Every
other active modern result construction identified below still uses
`fresh_tmp`, an equivalent direct `%t` increment, or a raw string returned by
`emit_rval_id`.

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
| `LirAbsOp`: `result,arg`; `int_type` | Active, PI `emit_post_builtin_call` | Text-only/monostate operands; result from `fresh_tmp` | Native `LirTypeRef`; kind/type only and ownership-ready | `lir_builtin_abs_value_chain_identity.c`; **generic scalar value** after representative PB seam |
| `LirIndirectBrOp`: `addr`; `targets` | Active, PS `emit_control_flow_stmt(IndirBrStmt)` | Address text-only from `emit_rval_id`; targets raw `vector<string>` | Address kind plus nonempty target vector; no target identity | Address could use generic value, but targets require `LirBlockId`; `lir_indirectbr_identity.c`; **separate CFG family** |
| `LirExtractValueOp`: `result,agg`; `agg_type,index` | Active in PX unary/cast paths, PB `emit_complex_binary_arith` / `emit_rval_payload(BinaryExpr)`, PI complex/overflow builtins, and PO `coerce` | Value operands text-only; result `fresh_tmp` | Native `LirTypeRef,int`; kind/type only and ownership-ready | Aggregate type/index validation is additional; `lir_extractvalue_chain_identity.c`; **generic result/use plus distinct aggregate semantics** |
| `LirInsertValueOp`: `result,agg,elem`; `agg_type,elem_type,index` | Active in PX unary paths, PB `emit_complex_binary_arith` / `emit_rval_payload(BinaryExpr)`, PI complex builtins, and PO `coerce` | Value operands text-only; result `fresh_tmp` | Native types/index; kind/type only and ownership-ready | `lir_insertvalue_chain_identity.c`; **generic result/use plus distinct aggregate semantics** |
| `LirLoadOp`: `result,ptr`; `type_str` | Active broadly in PR/PL/PX/PC/PV/PO; CC-LOAD-1 exact producer is PR selected-global branch | CC-LOAD-1 uses `fresh_value` result + global `LinkNameId`; all local/SSA/object routes use text-only operands and `fresh_tmp` | Native `LirTypeRef`; kind/type + ownership-ready; **741 exact** only for selected global | Preserve CC-LOAD-1 as **regression neighbor**. Other routes depend on local/object pointer ownership; probes per route, starting `lir_local_load_identity.c`; **separate pointer/object family** |
| `LirStoreOp`: `val,ptr`; `type_str` | Active in PL/PX/PC/PI/PV/PS/PO and PF parameter/local setup; CC-STORE-1 exact producer is PL `emit_set_assign_value` via `integer_store_operand_after_coercion` | CC-STORE-1 has native immediate + global `LinkNameId`; other routes text-only/monostate; no result | Native `LirTypeRef`; kind/type + ownership-ready; **741 exact** only for selected global integer | Preserve CC-STORE-1 as **regression neighbor**. SSA value use can share generic propagation, pointer needs object authority; `lir_local_store_identity.c`; **mixed generic/separate** |
| `LirMemsetOp`: `dst,byte_val,size`; `is_volatile` | Active, PL `emit_store_assignable_value` zero aggregate and PS `emit_non_control_flow_stmt(LocalDecl)` | Text-only/monostate operands; no result | Native bool; verifier kinds only | `lir_memset_native_use_identity.c`; **separate memory/object family** |
| `LirCastOp`: `result,operand`; `kind,from_type,to_type` | Active in PX/PB/PL/PC/PI/PV/PO and PF fixed-vector parameter setup; representative scalar casts from PX `emit_rval_payload(CastExpr)` | Text-only/monostate operands; result `fresh_tmp` | Native cast enum and type refs; verifier checks kinds/types but not cast legality/ownership | Candidate representative non-call chain neighbor; `lir_scalar_cast_chain_identity.c`; **generic value** plus distinct cast legality |
| `LirGepOp`: `result,ptr,indices`; `element_type,inbounds` | Active in PR/PL/PX/PC/PV and PF parameter setup; CC-GEP-1 exact producer is PR selected-global array branch | CC-GEP-1 uses `fresh_value`, global `LinkNameId`, typed native indices; other routes use `fresh_tmp`, raw base, often raw `LirGepIndex` presentation | Native type/bool; ownership-ready; **741 exact** only for selected-global typed path | Preserve CC-GEP-1 as **regression neighbor**. Other paths require object/base and typed-index work; `lir_local_gep_identity.c`; **separate pointer/object family** |
| `LirCallOp`: `result`; `callee,direct_callee_link_name_id`; `structured_args[].operand`; typed signature/type/ext/ABI fields; text mirrors | Active, PC: `prepare_call_args`, `emit_void_call`, `emit_call_with_result`, `make_lir_call_op_with_return_type_ref` | Structured direct integer result uses `fresh_value`; the common `OwnedLirTypedCallArg` now stores `LirOperand`. The focused direct void fixed integer literal preserves its native immediate and exact type ref when coercion keeps the representation; other arguments remain monostate compatibility | Direct integer result has exact ID ownership. The focused direct void fixed immediate row now has exact structured type/payload/range/count/ext verification and bypasses presentation parsing only when complete | Steps 3-4 close scalar result and fixed immediate rows. SSA reuse remains Step 5 on the same carrier; indirect/variadic/ABI/aggregate rows and intrinsic results remain unclaimed; text mirrors are presentation-only |
| `LirBinOp`: `result,lhs,rhs`; `opcode,type_str` | Active, PB scalar/complex arithmetic and logical helpers; also PL compound assignment, PI builtins, PV, PS | All value operands text-only; result `fresh_tmp` | Native opcode ref/type ref; kind/type only and ownership-ready | Preferred Step-2 representative `lir_scalar_ordinary_value_chain_identity.c`; **generic scalar result/use** |
| `LirCmpOp`: `result,lhs,rhs`; `is_float,predicate,type_str` | Active, PB comparisons/logical, PI FP/builtin checks, PV, PS loop/range lowering, `core.cpp` helpers | Text-only operands; result `fresh_tmp` | Native predicate/type/bool; kind/type only and ownership-ready | Depends on generic scalar seam; `lir_scalar_cmp_chain_identity.c`; **generic scalar value** plus predicate agreement |
| `LirPhiOp`: `result`; `incoming[value,label]`; `type_str` | Active, PX `emit_rval_payload(TernaryExpr)`, PB `emit_logical`, PV AArch64/AMD64 joins | Result `fresh_tmp`; incoming value and predecessor are raw strings | Native result type only; result kind/nonempty incoming; incoming entries are not visited for value ownership | Result can share generic allocation, but incoming values need `LirOperand` and predecessors need `LirBlockId`; `lir_phi_identity.c`; **distinct value+CFG carrier** |
| `LirSelectOp`: `result,cond,true_val,false_val`; `type_str` | Active, PI `emit_builtin_ffs_call` | Text-only operands; result `fresh_tmp` | Native type; kind/type only and ownership-ready | Depends on generic scalar seam; `lir_select_chain_identity.c`; **generic scalar value** |
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

- call argument preparation beyond the Step-4 immediate row: the common
  `LirOperand` carrier exists, but SSA propagation and ABI/aggregate transforms
  retain phased monostate compatibility;
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
3. **Void SSA argument**: the selected-global load already returns the CC-LOAD-1
   `LirValueId`; `prepare_call_arg` still falls back to a monostate operand for
   this unclaimed row. Step 5 must reuse the common carrier introduced for seam
   2 rather than duplicate it.
4. **Non-call scalar chain**: PB emits `LirBinOp` results and later operands
   through `fresh_tmp`/strings with native opcode/type already present. A
   two-operation integer chain isolates generic result allocation/use
   propagation without calls, CFG, locals, parameters, or ABI work.

Thus Step 2 may proceed. Seam 3 has an explicit implementation dependency on
the common call-argument carrier established by seam 2, but its probe and first
bad fact are independent; this is sequencing, not a hidden prerequisite.

## Step-2 focused probe bindings

These probes record the current production boundary only. They deliberately
pass today and do not implement a carrier, schema, producer, or verifier rule.
The matching structural observations live in
`frontend_lir_call_type_ref_test.cpp` and select LIR alternatives directly;
rendered LLVM output is not used as identity evidence.

| Probe and stable first bad fact | Producer and exact carrier transition | Required verifier rejection after publication | Forbidden fallback |
|---|---|---|---|
| `lir_direct_scalar_result_call_identity.c`: Step-2 first bad fact closed; the sole structured direct i32 `LirCallOp.result` and its return use now carry the same valid `LirValueId` | PC `emit_call_with_result`: `fresh_value(ctx)` -> operand-taking `make_lir_call_op_with_return_type_ref(LirOperand)`; `emit_rval_call_operand` returns the identical operand to the existing return path. Direct target `LinkNameId`, native i32 return ref, and structured signature remain unchanged | Implemented: reject missing/invalid/duplicate result IDs, unknown/cross-function downstream IDs, and any void-result authority through exact call checks plus generic current-function ownership | `%tN`, call formatting, callee spelling, and instruction order remain presentation only; misleading producer/use displays with the same ID verify successfully |
| `lir_direct_void_immediate_arg_identity.c`: Step-2 first bad fact closed; the sole fixed i32 structured argument carries `LirIntegerImmediate{7}`, exact i32 type refs, `None` extension, and no call result | PR `emit_rval_operand` -> PC representation-preserving fixed integer coercion -> `OwnedLirTypedCallArg::operand: LirOperand` -> authority-preserving `lir_call_structured_args`; formatting reads only `operand.str()` | Implemented authority-first rejection for missing payload, wrong/invalid alternatives, range, type/signature/count/ext conflicts; complete structured authority ignores misleading argument/type presentation | `operand`, `args_str`, raw argument/parameter type spellings, and suffix text are never reparsed to create or repair authority |
| `lir_direct_void_ssa_arg_identity.c`: CC-LOAD-1 produces a valid selected-global load result ID, but the sole call argument is still classified `SsaValue` with no ID | PR selected-global `emit_rval_operand` returns authoritative `LirOperand`; PC now has the common operand carrier but deliberately falls back to monostate outside the Step-4 immediate claim | Step 5 must reuse this carrier; reject unknown/cross-function argument IDs and require the argument ID to equal an already registered current-function definition. Keep selected-global load verification unchanged | Never compare the load/result and argument displays, scan `%tN`, or add an SSA-only side carrier |
| `lir_scalar_ordinary_value_chain_identity.c`: both PB binary results are classified SSA text without IDs, so the second operation's SSA lhs cannot retain the first result authority | PB scalar `emit_rval_payload(BinaryExpr)`: native Add/Mul opcode refs and i32 type refs surround `fresh_tmp` result strings and string operands; the selected-global source load remains a CC-LOAD-1 neighbor | Register each authoritative binary result exactly once; reject invalid/duplicate result IDs and unknown/cross-function authoritative operands; require the second lhs ID to resolve to the first binary definition | Never infer producer/use equality from shared `%tN` spelling, opcode text, rendered LLVM, or adjacency |

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
  and authority-preserving structured construction. Other argument rows fall
  back to monostate compatibility without parsing presentation;
- representative scalar chain: `expr/binary.cpp` constructs `LirBinOp` from
  `fresh_tmp` results and string operands while carrying native
  `LirBinaryOpcodeRef`/`LirTypeRef`;
- 741 neighbors: `expr/coordinator.cpp:474,497` populate selected-global
  `LirGepOp`/`LirLoadOp`; Step 3 adds the direct integer-call result as the
  third `fresh_value` production site;
  `lvalue.cpp:399` retains the authoritative store RHS operand, and
  `stmt.cpp:381` retains the authoritative return operand.
