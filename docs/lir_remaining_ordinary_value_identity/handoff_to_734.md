# Ordinary Value-Identity Handoff To Idea 734

This is the Plan-Step-8 handoff from
`ideas/open/744_lir_remaining_ordinary_value_identity_publication.md` to
`ideas/open/734_lir_to_new_bir_container_completeness.md`. It records only
producer and LIR-verifier facts. It neither creates a BIR container nor claims
that the current importer accepts an ordinary LIR instruction.

## Checked inventory

The mechanical inventory in `authority_matrix.md` was re-run at this handoff:

```sh
sed -n '/using LirInst = std::variant</,/^>;/p' src/codegen/lir/ir.hpp \
  | rg -o '^    Lir[A-Za-z0-9]+' | sed 's/^    //' | sort -u
sed -n '/## Mechanical 38-alternative matrix/,/## Separate adjacent/p' \
  docs/lir_remaining_ordinary_value_identity/authority_matrix.md \
  | rg -o '^\| `Lir[A-Za-z0-9]+' | sed 's/^| `//' \
  | rg -v '^LirInst$' | sort -u
comm -3 <source-list> <matrix-list>
```

The source and matrix lists each contain **38** alternatives and `comm -3` is
empty. The inventory remains **12 producerless legacy + 26 active modern**
alternatives. The matrix is the complete disposition for those alternatives;
the rows below name only bounded source shapes whose producers and verifier
obligations are already concrete enough for an idea-734 receiver packet.

## Exact receiver-ready source rows

Each advertised result definition is allocated by its owning `LirFunction`
through `fresh_value`, and every advertised use `LirValueId` resolves in that
same function; every `LinkNameId` is a module identity; and every
`LirIntegerImmediate` is a native payload. Display spellings, instruction
order, and emitted LLVM text are not receiver inputs.

| Source row and exact fields/carriers | Producer guarantee | LIR verifier obligation and focused proof | Exact idea-734 receiver work |
|---|---|---|---|
| 741 regression neighbors: selected-global scalar `LirLoadOp{result: LirValueId, ptr: LinkNameId, type_str: LirTypeRef}`; selected-global array-decay `LirGepOp{result, ptr, indices, element_type, inbounds}`; selected-global integer `LirStoreOp{ptr, val: LirIntegerImmediate, type_str}`; scalar integer `LirRet{value: LirIntegerImmediate or LirValueId, type_str}` | Existing PR/PL/statement seams retain those exact IDs, global identity, typed indices/immediate, and return type; no local/object form is implied. | Existing 741 focused frontend/backend guards reject invalid, unresolved/ambiguous global, ownership, type, and immediate-range faults. | Reuse only these exact Load/GEP/Store/Return shapes: map module global and function values, materialize immediate values, build ordered typed GEP indices, verify BIR result/use/type edges, and commit transactionally. |
| `LirCallOp` resolved direct integer result: `result: LirValueId`, direct callee `LinkNameId`, `return_type`/`callee_signature.return_type_ref`, and structured signature; fixed-void immediate/SSA argument subrows use `structured_args[].operand: LirIntegerImmediate` or current-function `LirValueId`, exact argument refs, `None` extension. | PC `emit_call_with_result` allocates before call construction; fixed-argument preparation preserves only representation-stable native carriers. | Direct-result and fixed-void checks plus generic ownership reject missing/invalid/duplicate result IDs, unresolved/cross-function IDs, wrong callee/signature/count/type/ext/argument alternatives. Proof: `frontend_lir_call_type_ref`. | Define one typed direct-call receipt shape with result registry, callee-ID resolution, structured return/parameter types, and ordered argument use edges. Keep indirect, variadic, ABI-expanded, aggregate, and nonmatching coercion forms unsupported. |
| `LirCallOp` native builtin rows: i32/i64 `Cttz`/`Ctlz`/`Ctpop` result IDs, intrinsic `LinkNameId`, native intrinsic kind, exact fixed integer signature/refs, `Cttz`/`Ctlz` zero behavior + i1 flag, or absent Ctpop behavior; i64 paths feed exact `LirCastOp Trunc` result. | PI builds calls and their result IDs with `fresh_value`; i64 narrowings allocate a distinct native Trunc and preserve the call ID as source. | Intrinsic/signature/behavior/count/type and generic ownership checks reject malformed call, narrowing, and final-use chains. Proof: focused ctz/clz/popcount cases in `frontend_lir_call_type_ref`. | Add a separately tagged intrinsic-call import path only after Raw-BIR represents kind and zero-count semantics; import i64 narrowing as a typed cast edge. Do not treat intrinsic spelling as identity. |
| Resolved fixed-void native floating `LirCallOp`: `result: LirValueId`, global callee/direct `LinkNameId`, matching native floating return and empty fixed-void signature; the exact result is the downstream double `LirBinOp FAdd` lhs. | The normal bodyless-extern `hir::Function` route reaches the same PC `emit_call_with_result` seam and returns its one `fresh_value` operand to `FAdd`. | The call must resolve to exactly one module Function with matching fixed-void structured signature; call and generic ownership checks reject missing/duplicate declaration identity, signatures, result IDs, unknown use, and FAdd type conflict. Proof: `test_block_scope_extern_void_prototype_uses_direct_function_entity` in `frontend_lir_call_type_ref`. | Receiver may accept only this resolved, zero-argument native-floating direct Function subrow, preserving declaration/callee identity, signature, result definition, and FAdd use. Indirect, variadic, argument-bearing, ABI, aggregate/object, and unproven floating forms stay unsupported. |
| Ordinary scalar `LirBinOp`: Step-6 normalized integer and Step-7.5 floating arithmetic `result/lhs/rhs: LirValueId` when available, native `opcode` and `type_str`; Step-7.14 ffs add-one preserves Cttz result on lhs plus representable immediate one. | PB uses `fresh_value` only after complex/vector/pointer/logical branches exit and returns the same operand to the focused downstream use. | Opcode/type/operand authority plus generic ownership reject invalid/duplicate definitions, unknown/cross-function uses, and native opcode/type conflicts. Proof: ordinary scalar and ffs focused frontend cases. | Add typed scalar binary receipt keyed by native opcode/type and source IDs/immediates; keep literal-presentation, compound, complex, vector, pointer/object, logical-helper, and other builtin rows unsupported. |
| Explicit scalar `LirCastOp`: integer width-change, FPTrunc/FPExt, SIToFP/UIToFP, FPToSI/FPToUI; exact `result` and authoritative source `operand` IDs with native `kind`, `from_type`, and `to_type`. Also the bounded ffs/ctz/clz/popcount i64-to-i32 Trunc links. | PX's operand-returning coercion path allocates only the proven scalar shape through `fresh_value` and preserves the source carrier. | Cast endpoint/kind checks plus generic ownership reject invalid definitions, unknown/cross-function sources, and endpoint/kind conflicts. Proof: focused scalar-cast and builtin-narrowing cases. | Add typed cast receipt only for these exact endpoint/kind families and preserve source/result registry edges; leave implicit, pointer, bitcast, vector, complex, aggregate, and unlisted builtin casts unsupported. |
| Ordinary scalar `LirCmpOp`: PB integer or floating result ID with native mode, predicate, and compared `type_str`; PI ffs equality-to-zero result ID is the matching `LirSelectOp` condition. | PB and the bounded PI ffs route allocate with `fresh_value` and preserve the result into their existing consumer. | Native predicate/mode/type checks and generic ownership reject malformed definitions/uses and float/integer conflicts. Proof: focused scalar-compare and ffs condition cases. | Add typed compare receipt with exact predicate/mode/type and result edge; retain pointer/vector/complex/logical-helper/vaarg/statement comparisons as unsupported. |
| PI builtin-ffs `LirSelectOp`: i32 direct or i64-to-Trunc result, exact scalar type, native zero immediate, current-function comparison condition, and add-one false arm. | PI allocates select with `fresh_value` and retains the exact Cttz/add-one/comparison edges. | Select checks require its native result and comparison-defined condition; generic ownership rejects invalid/duplicate/unknown edges. Proof: focused i32/i64 ffs chains. | Add one typed ffs-select receipt family only when the receiver can represent its result, comparison condition, arms, and immediate without text parsing. |
| PI integer `LirAbsOp`: `result: LirValueId`, argument is a current-function SSA ID or representable integer immediate when structurally available, and exact integer `int_type`. | `emit_post_builtin_call_operand` uses `fresh_value` and representation-preserving coercion for the bounded abs/labs/llabs route. | Exact integer type/argument alternative and generic ownership reject missing/invalid/duplicate/unknown/cross-function cases. Proof: `lir_scalar_abs_result_use_identity` coverage in `frontend_lir_call_type_ref`. | Add a tagged integer-abs receipt shape with typed input/result; do not generalize to noninteger, aggregate, vector, or arbitrary call routes. |
| PS scalar output-only `LirInlineAsmOp` binding subrow: `ordinary_results[0].value: LirValueId`, exact i32 or i64 binding type, `Output` role, index zero, and one type-matched Store use; compatibility `result` and all assembly/constraint/clobber text are non-authoritative. | The output-only i32/i64 path allocates the semantic binding through `fresh_value` while retaining compatibility display independently. | Binding shape/role/index/type and generic ownership reject malformed/duplicate outputs and wrong/unknown/cross-function Store uses. Proof: focused i32/i64 inline-asm frontend coverage. | Add an explicit semantic-output binding container only if 734 deliberately scopes inline asm; never import opaque strings as value authority. Read/write, inputs, memory, multi-output, explicit-register, vector/aggregate, and `insn_r` forms remain unsupported. |

## Fail-closed inventory

No receiver work is authorized from text-only or merely ownership-ready fields.
The following matrix rows remain fail-closed until a separate producer packet
publishes their missing carrier and the receiver owns a matching BIR shape:

- **Producerless legacy:** `LirConstInt`, `LirConstFloat`, `LirLoad`,
  `LirStore`, `LirBinary`, `LirCast`, `LirCmp`, `LirCall`, `LirGep`,
  `LirSelect`, `LirIntrinsic`, and `LirInlineAsm`.
- **Memory/object, stack, va-list, or CFG families:** `LirMemcpyOp`,
  `LirMemsetOp`, `LirVaStartOp`, `LirVaEndOp`, `LirVaCopyOp`, `LirVaArgOp`,
  `LirStackSaveOp`, `LirStackRestoreOp`, `LirAllocaOp`, `LirIndirectBrOp`,
  and all local/SSA/object `LirLoadOp`/`LirStoreOp`/`LirGepOp` forms.
- **Aggregate/vector/CFG-carrier gaps:** `LirExtractValueOp`,
  `LirInsertValueOp`, `LirInsertElementOp`, `LirExtractElementOp`,
  `LirShuffleVectorOp`, and `LirPhiOp`; aggregate/vector operands, typed
  indices/masks, and phi predecessor/value pairs need distinct carriers.
- **Unproven alternatives inside otherwise named operations:** indirect,
  variadic, argument-bearing floating, ABI-expanded, aggregate/object, and
  unresolved `LirCallOp`; nonfocused binary/cast/compare/select/abs producers;
  all opaque inline-asm payloads and unproved bindings; body parameters;
  noninteger return values; and CFG labels/terminator target text.

Idea 734 must continue to reject these rows rather than recover values,
callees, types, or control flow from rendered strings.

## Proof and resume point

The source audit inspected the active producer seams in
`call/target.cpp`, `call/builtin.cpp`, `expr/binary.cpp`, `expr/misc.cpp`, and
`stmt.cpp`, plus `verify.cpp` call, scalar-op, and generic ownership paths.
Focused producer/verifier proof is `frontend_lir_call_type_ref`; the Step-7.32
extern double variants are in that executable. The canonical post-change
backend checkpoint is recorded in root `test_after.log` (4/4 `backend_` tests
passed).

Idea 734 should reactivate at its function-body importer boundary and take one
row from the table at a time: define the typed Raw-BIR payload and source-ID
mapping, add import dispatch and BIR verification, then prove success and
transactional failure. Completion of this handoff does not complete idea 734.
