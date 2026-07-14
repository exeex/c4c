# Current Packet

Status: Active
Source Idea Path: ideas/open/776_lir_typed_expression_result_carrier_decomposition.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Publish the PHI-carrier consumer handoff

## Just Finished

- Plan Step 4 complete: added `test_vaarg_helper_result_authority_loss_boundary`, an
  independent frontend-LIR probe for `__builtin_va_arg(ap, int) + 1` with semantic
  vaarg operations enabled. It verifies one typed `i32` `LirVaArgOp`, its SSA va_list
  pointer operand, and a later typed `i32` Add. The vaarg result and later Add left
  operand have neither `LirValueId` nor other `LirOperand` authority.
- First loss seam: `emit_rval_payload(VaArgExpr)` allocates the `LirVaArgOp` result
  with `fresh_tmp` and passes it to `emit_lir_op` as raw text; it returns that string,
  and `emit_rval_expr` wraps payload results as raw operands. The required repair
  direction is a typed vaarg result carrier from helper allocation through
  `emit_lir_op`, the expression-result boundary, and its later consumer. No production
  repair, PHI carrier, generic expression migration, ternary coverage, or logical
  coverage is claimed.

## Suggested Next

- Begin Plan Step 5 by publishing the PHI-carrier consumer handoff from the separate
  ternary, logical, and vaarg observations; do not implement a PHI/carrier repair.

## Watchouts

- The vaarg result is first lost before the expression coordinator; repairing it may
  require a typed expression-result carrier, but this probe does not authorize that
  migration or any PHI work.
- Keep the ternary, logical, and vaarg first-loss seams independent in the Step 5
  handoff.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R
  '^frontend_lir_call_type_ref$'` passed. The delegated packet forbids modifying the
  supervisor-owned `test_after.log`; no root regression log was written by this packet.
