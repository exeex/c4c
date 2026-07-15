# Current Packet

Status: Active
Source Idea Path: ideas/open/817_lir_body_parameter_receiver_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish and verify the selected authority contract

## Just Finished

- 817 Step 1 selected exactly one row: a direct non-expanded pointer
  parameter used as the `LirGepOp.ptr` SSA base for `p[0]` (for example,
  `int first_byte(char *p) { return p[0]; }`).  The producer allocates one
  `LirValueId` and publishes
  `LirCurrentFunctionBodyParameterDefinition{value, parameter_index,
  LirTypeRef(Pointer), owner=function.link_name_id}` in
  `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:1406-1420`; declaration names,
  `params`/signature text, raw operands, and diagnostics are excluded from
  authority. `emit_decl_ref_rval_operand` returns that same SSA ID for the
  body use at `src/codegen/lir/hir_to_lir/expr/coordinator.cpp:650-664`, and
  `emit_indexed_gep` carries it in `LirGepOp.ptr` with a typed i64 immediate
  index at `src/codegen/lir/hir_to_lir/lvalue.cpp:997-1033`.
- The required ABI classification is the narrow `native direct pointer`
  producer predicate (pointer, non-array; excluding AMD64 fixed-aggregate
  byval, AArch64 HFA, and AArch64 vector-as-i32) at
  `hir_to_lir.cpp:1406-1411`. It is currently only implicit in lowering, so
  Step 2 must publish/check that exact class rather than infer it from a
  signature or spelling. `verify_native_body_parameter_definitions` already
  checks defined current owner, value, parameter index, pointer type, and
  signature-type coherence, and rejects duplicate indices/value IDs
  (`src/codegen/lir/verify.cpp:2116-2145`); it must receive focused malformed,
  foreign, duplicate, and ABI/type-incoherent coverage for this one row.

## Suggested Next

- Execute Step 2 only: publish and verify the selected direct-pointer
  `LirGepOp.ptr` body-use contract, including explicit native-direct ABI
  classification and focused malformed/foreign/duplicate/type-incoherent
  boundary coverage.

## Watchouts

- Open 795 is complete for an unrelated baseline return, not a 734 handoff.
  The selected row is only direct non-expanded pointer `p[0]`/typed-GEP-base
  use; scalar, byval/aggregate, HFA/vector, array, variadic, and all other
  parameter forms remain fail-closed. Do not treat declaration facts,
  parameter names, signatures, operands, or diagnostics as body-use authority;
  do not edit Raw-BIR or 734 receiver code.

## Proof

- No test command required for this read-only Step 1 trace. Targeted
  `./build/c4cll --codegen llvm` on a temporary `p[0]` fixture confirmed the
  concrete LLVM-path shape; Step 2 must select and run its fresh focused
  producer/verifier proof before a handoff is accepted.
