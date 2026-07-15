# Current Packet

Status: Active
Source Idea Path: ideas/open/785_lir_call_result_operand_carrier_foundation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Bind and validate the minimal call-result carrier

## Just Finished

- Step 2 complete. The FP/alignment-only `@llvm.ptrmask.p0.i64` call at
  `hir_to_lir/call/vaarg.cpp:145` now receives `fresh_value(ctx)` and uses the
  existing `make_lir_call_op_with_return_type_ref` with native pointer return
  type. The sibling HFA ptrmask at `:244` remains unchanged.
- A native ptrmask input requires its immediate GEP consumer to be
  authoritative, so only the selected ptrmask branch now emits a native
  `stack_next` GEP with a typed i64 immediate index. Its store consumes that
  native GEP result. The <=8-byte branch retains its original raw GEP route.
- `test_aarch64_fp_vaarg_ptrmask_result_identity_boundary` lowers an AArch64
  `long double` vaarg fixture, verifies the ptrmask result has a valid native
  value ID and pointer return type, and checks a GEP pointer operand owns the
  exact same ID. It uses structural fields, not rendered result text.
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

- Return to the supervisor for the next runbook decision. This packet does not
  authorize conversion of the sibling HFA ptrmask route or other legacy call
  sites.

## Watchouts

- No `call_args_ops.hpp` API or support-type expansion is justified by this
  inventory. Keep the change to a named direct caller; do not broaden it to
  generic-expression, Raw-BIR/importer, backend, target lowering, MIR,
  emission, PHI, or unrelated vaarg helpers.

## Proof

- Fresh Step 2 proof passed 1/1: `cmake --build --preset default && ctest
  --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`.
  Full output is preserved in `test_after.log`.
