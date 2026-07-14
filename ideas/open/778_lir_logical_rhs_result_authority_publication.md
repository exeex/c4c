# LIR Logical RHS Result Authority Publication

Status: Open
Type: one-family typed expression-result producer successor
Unblocks: `ideas/open/775_lir_phi_producer_helper_result_identity.md`
Downstream Consumer: `ideas/open/751_lir_phi_incoming_value_and_predecessor_identity.md`
Evidence Source: closed `ideas/closed/776_lir_typed_expression_result_carrier_decomposition.md`

## Goal

Publish native owning current-function `LirValueId` authority for the logical
short-circuit RHS non-`i1` conversion result, without changing PHI carriers,
generic expression APIs, or any other family.

## Why This Exists

776's logical probe (`ed1b2dab5`) established that the RHS boolean conversion
enters `emit_logical` with typed authority, but its non-`i1` `zext` result is
allocated by `fresh_tmp` before the later raw PHI seam. That direct allocation
is independently bounded: it can use `fresh_value` and publish a native
`LirCastOp.result` without changing `emit_rval_payload`, `emit_rval_id`,
`coerce`, or `LirPhiOp`.

The PHI result and incoming pairs remain a separate receiver problem. This
source publishes only the RHS conversion producer fact needed for a later 775
handoff; it does not claim that the logical value reaches a PHI carrier.

## In Scope

- replace only the logical RHS non-`i1` conversion's `fresh_tmp` result with an
  owning current-function `LirValueId` allocated by `fresh_value`;
- retain that result as a native `LirCastOp.result` at the existing logical
  producer site;
- add focused frontend-LIR positive and malformed missing, invalid, duplicate,
  and foreign RHS conversion result-authority proof; and
- publish the exact logical-RHS-only handoff to 775.

## Out Of Scope

- logical PHI result/incoming entries, final logical consumer propagation,
  ternary/coerce, vaarg, generic `emit_rval_payload` / `emit_rval_id` /
  `coerce` migration, and all other expression families;
- PHI carrier/verifier, predecessor/edge work, text recovery, or 751 work;
- Raw-BIR/importer, backend, target lowering, MIR, emission, result-name maps,
  side tables, synthetic values, display-text parsing, and testcase-specific
  branches.

## Acceptance Criteria

- The selected RHS `LirCastOp` result has an owning valid current-function
  `LirValueId` before any display spelling is used.
- Focused native frontend-LIR coverage proves that result authority and proves
  malformed missing, invalid, duplicate, and foreign authority fails closed.
- The handoff names only the logical RHS conversion field, proof, and 775
  return point; the raw PHI result/incoming boundary remains explicitly
  unresolved.

## Reviewer Reject Signals

- Reject a generic expression API migration, a change to non-RHS logical
  results, or a ternary/vaarg change claimed as this logical producer repair.
- Reject PHI carrier/verifier, text-derived recovery, maps, side tables,
  synthetic values, or testcase-specific result creation.
- Reject Raw-BIR/importer/backend/target-lowering work, malformed-contract
  weakening, expectation downgrades, or baseline edits as capability progress.
