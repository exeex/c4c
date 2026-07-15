# 795 Step 1: selected parameter-index body-use trace

## Scope and observation

The delegated command was run from `/workspaces/c4c`:

```text
./build/c4cll --codegen llvm tests/c/external/gcc_torture/src/pr21173.c
error: LirGepOp.indices.value: authoritative GEP index requires integer or SSA authority
```

`--dump-hir` identifies the only selected function-body parameter route without
using LLVM rendering as authority:

```text
fn foo(p: char*) -> void  [entry: block#0]
  block#1 [term]:
    (a#G1[i#L0] += (p#P0 - (&q#G0)))
```

The selected source identity is therefore HIR `DeclRef p#P0` in `foo`, whose
logical parameter index is 0 and type is `char*`.  It is the pointer operand
of the RHS pointer difference; that difference is the variable index used by
the direct pointer-compound GEP for `a[i] += ...`.  The loop local `i#L0` is
only the separate array-selection index and is not selected by this route.

## Exact lowering route

1. `lower_hir_to_lir` in `src/codegen/lir/hir_to_lir/hir_to_lir.cpp` creates
   the body `LirFunction` and calls `populate_lir_function_params`,
   `populate_signature_type_refs`, then `init_fn_ctx` before emitting `foo`'s
   statements.  `populate_lir_function_params` publishes one logical
   `("%p.p", char*)` parameter; `populate_signature_type_refs` publishes one
   non-byval signature parameter.  The plain-fixed-scalar verifier requires
   those logical/signature/type-mirror tracks to agree, confirming this is a
   one-to-one native parameter form rather than an ABI-expanded carrier.
2. `init_fn_ctx` publishes only `ctx.param_slots[0] = "%p.p"` for this
   ordinary parameter.  Its ABI-special branches are AArch64 HFA and
   AArch64 fixed-vector-as-i32; neither applies to `char*`.  The AMD64 byval
   materialization branch is aggregate-only and does not apply either.
3. `StmtEmitter::emit_decl_ref_rval_operand` in
   `src/codegen/lir/hir_to_lir/expr/coordinator.cpp` consumes that slot.  For
   an unspilled, non-byval parameter it returns `LirOperand::raw(it->second)`.
   Thus the selected `p#P0` reaches body lowering as display `%p.p` with no
   `LirValueId` or current-function parameter authority.
4. `StmtEmitter::emit_binary_rval_operand` in
   `src/codegen/lir/hir_to_lir/expr/binary.cpp` lowers `p - &q` through its
   pointer-minus-pointer path.  It creates raw-name `LirCastOp` results for
   `ptrtoint`, a raw-name integer `LirBinOp` difference, and returns that
   derived integer spelling.  The direct pointer-compound branch then calls
   `coerce_operand` and `emit_indexed_gep` with this value.
5. `StmtEmitter::emit_indexed_gep` in
   `src/codegen/lir/hir_to_lir/lvalue.cpp` makes a typed authoritative GEP
   whenever its base has authority, but only treats an index as typed when it
   has an integer immediate or `LirValueId`.  The derived RHS has neither, so
   the emitted GEP has an authoritative result/base but a raw compatibility
   index.  `verify_authoritative_gep` in `src/codegen/lir/verify.cpp` then
   rejects exactly this mixed form at `LirGepOp.indices.value`.

## Classification and bounded seam

`p#P0` is **not a native integer parameter**.  It is a native, one-to-one
plain fixed-scalar **pointer** parameter (one logical parameter, one
non-byval signature parameter, one structured type mirror), not an
ABI-expanded/byval/HFA/vector/variadic form.  Its pointer-difference result is
the required i64 GEP-index authority.

The first missing native authority fact is at body publication/consumption:
the current `LirFunction` has no structured current-function parameter
definition for logical parameter 0, and `emit_decl_ref_rval_operand` discards
the slot as `RawText`.  The bounded repair seam is to add a checked
current-function native parameter definition for this one-to-one scalar
parameter surface, allocate/publish it while `init_fn_ctx` owns `foo`, and
return it as `LirOperand::ssa` at the parameter `DeclRef` consumer.  Step 2
must keep its proof to this pointer-to-integer-derived index chain and verify
foreign, malformed, type-incoherent, and display-derived parameter facts fail
closed.  The presently raw cast/bin results are the immediate downstream
continuation to audit after that publication; they are operator-result
propagation within this selected chain, not evidence to widen into ABI
families.

No separate parameter/ABI-family successor is required by this trace.  No
declaration-only identity, rendered parameter text, PHI route, or byval/HFA/
vector/variadic work was selected.
