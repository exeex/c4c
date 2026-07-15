# Current Packet

Status: Active
Source Idea Path: ideas/open/809_lir_ternary_phi_incoming_operand_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Retain same-type ternary PHI incoming operand authority

## Just Finished

- 809 Step 2 retains a returned SSA operand at the
  `StmtEmitter::emit_rval_payload(TernaryExpr)` PHI construction seam when a
  same-type coercion is a textual no-op. The local construction-side helper
  restores only a source `LirValueId`; void fallback, immediates, special
  tokens, and actual coercions continue through `then_coerced`/`else_coerced`.
  The parked 807 floating-`fneg` `fresh_value(ctx)` hunk remains unchanged.
- Nearby coverage retains the existing postfix ternary's load-producer ID
  assertions and extends `test_ternary_coerce_result_authority_boundary` with
  stale (`LirValueId::invalid()`) PHI-incoming rejection alongside its missing,
  unknown, and foreign authority cases.

## Suggested Next

- Supervisor: inspect the Step 2 slice and select the next active-plan packet.

## Watchouts

- Keep the parked 807 `fresh_value(ctx)` floating-`fneg` hunk unstaged and
  outside this 809 acceptance slice. The construction helper is deliberately
  local: do not generalize `coerce_operand` or alter PHI/verifier contracts.

## Proof

- `cmake --build --preset default` and
  `ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`
  (executor reports output; supervisor owns `test_after.log`).
