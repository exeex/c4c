# Current Packet

Status: Active
Source Idea Path: ideas/open/809_lir_ternary_phi_incoming_operand_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace the returned-operand to PHI-incoming construction seam

## Just Finished

- 809 Step 1 traced the parked 807 `fresh_value(ctx)` `fneg` evidence for
  `condition ? -input : 0.0`: `then_source`/`then_coerced` remain returned
  `LirOperand`s through `StmtEmitter::emit_rval_payload(TernaryExpr)` in
  `src/codegen/lir/hir_to_lir/expr/misc.cpp`. For same-type floating arms,
  the ternary's `coerce_operand(then_source, then_ts, res_spec)` call returns
  `LirOperand::raw(...)` through its no-conversion path in
  `src/codegen/lir/hir_to_lir/core.cpp`; the following `LirPhiOp` aggregate
  initializer (misc.cpp:277-282) faithfully stores that display-only operand
  in `LirPhiIncoming::value`. The bounded Step 2 seam is the ternary
  construction-side no-op-coercion handoff before its PHI incoming initializer,
  not `fneg` producer creation or `verify.cpp`.
- Nearby coverage is `test_ternary_coerce_result_authority_boundary` in
  `tests/frontend/frontend_lir_call_type_ref_test.cpp` (lines 4903-5006): add
  or adapt a floating `condition ? -input : 0.0` positive assertion that the
  selected `fneg` result ID is retained by its PHI incoming, alongside its
  existing missing, unknown, and foreign incoming-value rejection mutations.
  Add the missing stale case as an SSA incoming with
  `LirValueId::invalid()`; this exercises the existing invalid-ID rejection
  without changing the verifier or inventing display-name recovery.

## Suggested Next

- Execute 809 Step 2 only: make the smallest `misc.cpp` ternary
  no-op-coercion/`LirPhiOp::incoming` construction-side retention change (do
  not alter generic `coerce_operand`), then extend the nearby frontend test
  with the floating-`fneg` preservation assertion and existing-contract
  malformed-authority coverage direction.

## Watchouts

- Keep the parked 807 `src/codegen/lir/hir_to_lir/expr/misc.cpp` hunk intact
  but do not accept, enlarge, or move it. Do not alter producer creation, PHI
  schema/verifier behavior, other unary families, CFG semantics, or rendered
  text identity.
- `LirPhiIncoming::value` is already a `LirOperand`; the construction uses
  `void_to_zero` for void/fallback literals, so preserve its special-token and
  immediate behavior while retaining IDs only from returned SSA operands.

## Proof

- Discovery evidence only; this trace packet ran source/AST inspection and no
  build or test. Before acceptance, require a fresh build and focused blocker
  proof selected by the supervisor, recorded in `test_after.log`.
