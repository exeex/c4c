# Current Packet

Status: Active
Source Idea Path: ideas/open/785_lir_call_result_operand_carrier_foundation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory direct call-result carrier surfaces and probe feasibility

## Just Finished

- Inventory complete. `call_args_ops.hpp:87` already provides the direct
  native-result boundary: `make_lir_call_op_with_return_type_ref(LirOperand,
  LirTypeRef, ...)` trims only display text, then moves that operand into
  `LirCallOp.result` (`ir.hpp:387`). The legacy string wrapper at
  `call_args_ops.hpp:117` instead constructs a raw `LirOperand`, so it cannot
  carry a native value ID.
- The immediate direct loss sites are the `@llvm.ptrmask.p0.i64` calls at
  `hir_to_lir/call/vaarg.cpp:145` (FP source-pointer route) and `:244` (HFA
  route). Their results feed the nearby GEP/store/PHI or memcpy paths. Existing
  native factory users in `call/target.cpp:343` and `call/builtin.cpp:38`
  establish that a `fresh_value(ctx)` result is already supported.
- Smallest support-type candidate: no shared factory, argument-family, parser,
  or verifier change. A selected caller can use the existing native factory
  with `fresh_value(ctx)` and `LirTypeRef(LirBuiltinType::Pointer)`; the legacy
  wrapper remains for unrelated compatibility callers.
- Focused native-result probe is feasible beside
  `test_direct_scalar_result_call_identity_boundary` in
  `tests/frontend/frontend_lir_call_type_ref_test.cpp`: it already proves a
  native call result has a valid value ID and its direct consumer keeps that
  exact ID. Decision: this remains outside unrelated generic call/argument
  families. The clang AST declaration query returned both factory signatures;
  ccdb caller/type queries could not load the repository's build compile
  commands, so the direct callsite evidence above is source-local.

## Suggested Next

- Plan Step 2: convert only the explicitly selected ptrmask result site(s) to
  the existing native factory and add the focused structural result/consumer
  probe. Do not absorb the sibling HFA route unless that step names it.

## Watchouts

- No `call_args_ops.hpp` API or support-type expansion is justified by this
  inventory. Keep the change to a named direct caller; do not broaden it to
  generic-expression, Raw-BIR/importer, backend, target lowering, MIR,
  emission, PHI, or unrelated vaarg helpers.

## Proof

- Fresh Step 1 proof passed 1/1: `cmake --build --preset default && ctest
  --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`.
  Full output is preserved in `test_after.log`.
