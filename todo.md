# Current Packet

Status: Active
Source Idea Path: ideas/open/830_lir_direct_call_structured_argument_identity_prerequisite.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Produce and verify the structural argument relation

## Just Finished

- Step 1 traced the bounded producer gap. `hir_to_lir.cpp:1431-1444` assigns
  the second current-function direct-scalar parameter a native `LirValueId`
  and exact signature-derived `LirTypeRef`; `emit_decl_ref_rval_operand` then
  returns that value as `LirOperand::ssa`. In `call/args.cpp:375-400`, however,
  the preservation predicate is hard-coded to a one-parameter direct call.
  The selected two-parameter argument 1 therefore replaces its native source
  operand with `LirOperand(arg)` text. The preserved failed fixture in
  `test_after.log` demonstrates the resulting `args=2 types=0 ... operand=0
  value=none` gap (empty `arg_type_refs`, non-SSA `structured_args[1]`).
- Native construction does have the required seam: extend only that selected
  argument-1 predicate to carry the already-produced `source_operand` and a
  native `LirTypeRef` from `out_arg_ts` through `OwnedLirTypedCallArg` into
  existing `LirCallArg.operand`, `.type_ref`, and `arg_type_refs[1]`; the
  latter are copied by `call_args_ops.hpp:43-114`, not recovered from call
  text. This records no 829 parameter-authority tuple.

## Suggested Next

- Step 2 only: for one direct global, non-variadic, specified two-parameter
  call at argument index 1 whose unconverted source is a current-function
  `DirectScalar` SSA value, preserve that operand/type at
  `prepare_call_arg`. Add native verifier checks at the existing call seam:
  direct-global/link-name coherence; exactly two fixed parameters; index 1;
  SSA value validity/current-function ownership; nonempty equal
  `structured_args[1].type_ref`, `arg_type_refs[1]`, and fixed parameter-1
  type. Missing/incoherent relation must reject; all other forms remain out
  of contract. Do not publish `FixedDirectCallArgument1` or any 829 tuple.

## Watchouts

- Keep 830 limited to the direct/non-variadic/specified argument-1 native
  relation. Do not touch Raw-BIR, 734, generic call arguments, or unrelated
  821/822 material.
- `args_str`, printed operand/type strings, and `callee_signature` display
  strings may be checked only as existing compatibility mirrors; they cannot
  be the producer source. The native sources are `source_operand` and
  `out_arg_ts`; the fixed parameter-1 `LirTypeRef` is verifier coherence only.

## Proof

- No proof has been accepted for 830. Preserve `test_before.log` and
  `test_after.log`; the latter records 829's failed attempted fixture.
