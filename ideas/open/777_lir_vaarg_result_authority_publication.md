# LIR Vaarg Result Authority Publication

Status: Open
Type: one-family typed expression-result producer successor
Unblocks: `ideas/open/775_lir_phi_producer_helper_result_identity.md`
Downstream Consumer: `ideas/open/751_lir_phi_incoming_value_and_predecessor_identity.md`
Evidence Source: closed `ideas/closed/776_lir_typed_expression_result_carrier_decomposition.md`

## Goal

Publish native owning current-function `LirValueId` authority for the semantic
vaarg helper result and its immediate expression consumer, without changing
any other expression family or PHI representation.

## Why This Exists

776's accepted vaarg probe (`0b4cdc019`) proves that a semantic `LirVaArgOp`
retains its `i32` result type and SSA va-list pointer operand, but its result
is allocated with `fresh_tmp`, passed to `emit_lir_op` as raw text, returned by
the vaarg payload helper as a string, and wrapped raw by the expression
boundary. The later typed Add consequently has no result authority to consume.

Unlike ternary/coerce and logical, this route has one directly modeled native
producer (`LirVaArgOp`) and can be entered through a VaArgExpr-specific operand
path. That bounded entry point can publish `LirOperand::ssa` authority without
migrating all `emit_rval_payload`, `emit_rval_id`, or `coerce` callers.

## In Scope

- add a vaarg-only typed operand/result helper at the VaArgExpr first-loss
  boundary;
- allocate the semantic `LirVaArgOp` result with an owning current-function
  `LirValueId` before `emit_lir_op` receives it;
- route only VaArgExpr evaluation and its immediate typed consumer through
  that native `LirOperand` result;
- add focused frontend-LIR positive coverage plus malformed missing, invalid,
  duplicate, and foreign-result authority coverage for this vaarg route; and
- publish the exact one-family handoff back to 775.

## Out Of Scope

- ternary/coerce, logical, calls, ordinary expression APIs, generic
  `emit_rval_payload` / `emit_rval_id` / `coerce` migration, and other vaarg
  target-lowering families;
- `LirPhiOp` representation, PHI incoming verification, predecessor/edge
  work, text-derived PHI recovery, or 751 implementation;
- Raw-BIR/importer, backend, target lowering, MIR, emission, result-name maps,
  side tables, synthetic values, display-text parsing, and testcase-specific
  branches.

## Acceptance Criteria

- The selected semantic vaarg helper allocates and publishes a valid owning
  current-function `LirValueId` as its native `LirVaArgOp.result` before any
  display spelling is used.
- The immediate vaarg expression consumer retains that exact native result
  authority; no `%t`, rendered LLVM, instruction order, or side mapping is
  used to recover it.
- Focused frontend-LIR positive coverage proves the typed vaarg result/use
  chain, and malformed missing, invalid, duplicate, and foreign result
  authority fails closed.
- The handoff identifies only the vaarg typed field, proof, and 775 return
  point. It makes no PHI, other-family, generic-migration, or backend claim.

## Completed Vaarg-Only Handoff

Disposition: capability-complete for this one-family vaarg source; close
accepted pending supervisor lifecycle review. No ternary, logical, PHI, or
broader producer capability is claimed.

Scalar AMD64 semantic `LirVaArgOp.result` is allocated by `fresh_value` as an
owning current-function `LirValueId`, passed to `emit_lir_op` as
`LirOperand::ssa`, carried by the VaArgExpr-specific `LirOperand` route, and
consumed by the immediate typed Add with the exact same native value ID.

Focused proof
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure
-R '^frontend_lir_call_type_ref$'` passed. The native verifier rejects missing,
invalid, duplicate, and foreign vaarg result authority cases. The supervisor
accepted the regression guard and full baseline review for `55c499775`.

Exact 775 return point: 775 may consume only this vaarg typed-result handoff.
Ternary/coerce and logical producer authority remain unresolved, and PHI/751
remain blocked and out of scope. This handoff does not authorize generic
expression migration, nonsemantic vaarg target-lowering work, text recovery,
result-name maps, side tables, PHI carrier/verifier work, or
Raw-BIR/importer/backend work.

## Reviewer Reject Signals

- Reject any broad `emit_rval_payload`, `emit_rval_id`, or `coerce` migration
  claimed as a vaarg-only repair.
- Reject text parsing, `%t` matching, instruction-order lookup, result-name
  maps, side tables, synthetic values, or testcase-specific routes.
- Reject PHI carrier/verifier, Raw-BIR/importer/backend, target-lowering, or
  non-vaarg expression changes folded into this successor.
- Reject positive-only coverage, malformed-contract weakening, expectation
  downgrades, or baseline edits in place of native fail-closed authority proof.
