# Step 2: Label-address table producer map

## Scope and reading rule

This is a source-form-neutral frontend-LIR map, not a repair choice.  A
"label address" below means the HIR `LabelAddrExpr`; source spelling,
including whether a table appears in a larger integration program, does not
create another form.  The map stops before the table-element GEP/load result:
the structured result contract for that endpoint remains the accepted 767
contract and is not reopened here.  765 (member/bitfield RHS identity), 766
(SSA indexed-GEP pointer result), and 767 (static-local/local table-element
GEP/load pointer results) remain preserved prerequisites.

## Producer forms

| Form | Direct frontend-LIR producer and structured candidate | Input/result contract to probe | First authority boundary |
| --- | --- | --- | --- |
| Static-storage table initializer (file-scope or block-scope `static`; scalar and each aggregate table element share this producer) | `ConstInitEmitter::emit_const_array` recursively calls `emit_const_init`, ending in `emit_const_scalar_expr(LabelAddrExpr)`.  Its current output is a constant initializer spelling `blockaddress(...)`, not a `LirOperand` or `LirValueId`. | Input: a `GlobalInit` scalar label address in a pointer element, including nested aggregate placement.  Result candidate: retain a structured constant-label-address/initializer node that carries function and label identity, rather than infer it from rendered text. | Constant initialization serializes the label address before there is an operand/value identity.  This is distinct from later table access; it does **not** authorize changing static table-element GEP/load behavior. |
| Automatic local scalar initializer | `StmtEmitter::emit_non_control_flow_stmt(LocalDecl)` obtains the initializer through `emit_rval_id`; `emit_rval_operand` falls through `emit_rval_expr`, whose `LabelAddrExpr` route wraps `emit_rval_payload` in `LirOperand::raw`.  The local declaration then builds `LirStoreOp` from the returned string. | Input: a local pointer initialized directly from `LabelAddrExpr`.  Result candidate: an initializer operand that carries label-address authority into the store (or a deliberately structured constant operand), with type `ptr`; the probe must separately observe any later load result. | The direct rvalue producer has already become raw before the local store.  A later structured load, when reached, cannot establish where the label-address identity was originally lost. |
| Automatic local table representation/decay before element access | `emit_decl_ref_rval_operand(DeclRef local)` handles an array local through the string `emit_lir_op(LirGepOp{tmp,...})` overload and returns `LirOperand::raw(tmp)`.  It is the representation handoff from local table object to pointer-to-first-element. | Input: a local array whose pointer elements are label addresses, consumed as an array expression before indexing.  Result candidate: an authoritative table-base/decay GEP operand, with the local slot and zero indices represented structurally. | Local array decay returns a raw pointer presentation.  This boundary precedes, and must be kept separate from, the accepted table-element GEP/load result contract. |
| Static-storage table representation before element access | `emit_rval_operand(DeclRef global)` recognizes an array global and emits `LirGepOp` with `LirOperand::global(...)`, a fresh value, and typed zero indices. | Input: a static-storage table declaration reference used as an array base.  Result candidate: the existing structured global-base/decay GEP operand. | No first missing boundary is established by this form at the representation handoff: it already has a global operand and a fresh value.  The next endpoint is the preserved 767 table-element GEP/load contract, so this row is a control form, not a repair target. |
| Direct label-address rvalue consumption (outside an initializer) | `emit_rval_expr` has no dedicated `LabelAddrExpr` operand route and returns `LirOperand::raw(emit_rval_payload(...))`; `emit_rval_payload(LabelAddrExpr)` produces `blockaddress(...)`.  `resolve_payload_type(LabelAddrExpr)` supplies the `void *` type only. | Input: a `LabelAddrExpr` used directly as an rvalue, including the expression that feeds a local initializer or a later pointer operation.  Result candidate: a typed, structured label-address operand whose identity is available to its immediate consumer. | The direct producer emits presentation text with no value authority.  This is the earliest common rvalue boundary; it is not permission to patch the indirect-branch carrier, which already copies its operand ID exactly. |

## Evidence queries and narrow spans

The map is based on AST-backed queries, then only these source spans:

- `c4c-clang-tool-ccdb find-definition .../expr/misc.cpp emit_rval_payload .../build/compile_commands.json` identifies the `LabelAddrExpr` overload at line 297; `misc.cpp:297-301` shows its `blockaddress` payload.
- `c4c-clang-tool-ccdb find-definition .../const_init_emitter.cpp emit_const_scalar_expr .../build/compile_commands.json` identifies the constant scalar producer at line 615; `const_init_emitter.cpp:1018-1053` and `1122-1182` show the label branch and array recursion.
- `c4c-clang-tool-ccdb find-definition .../lvalue.cpp emit_rval_from_access_ptr .../build/compile_commands.json` identifies the structured access/load overload at lines 974 and 998; `lvalue.cpp:943-1010` shows the authority gate and fresh result.
- `c4c-clang-tool-ccdb find-definition .../stmt.cpp emit_control_flow_stmt .../build/compile_commands.json` identifies the indirect-branch overload at line 606; `stmt.cpp:606-625` confirms the carrier copies `addr.value_id()` exactly.
- `coordinator.cpp:445-539` and `585-662`, plus `stmt.cpp:121-192`, provide the narrow coordinator/local-declaration paths used above.

The attempted direct-callee queries for overloaded member names report that the
target is not uniquely found/called in a translation unit; the definition
queries above identify the overload sites, so no raw-text-wide substitute was
used.

## Step 3 extraction boundary

Extract focused frontend-LIR probes for exactly the five rows: static
initializer, automatic scalar initialization, automatic-table decay,
static-table decay control, and direct rvalue consumption.  Each probe must
assert its stated structured input/result candidate plus a nearby malformed
contract, and may not use emitted LIR text, external integration case names,
carrier publication, verifier relaxation, or a reimplementation of
765/766/767 as its authority.
