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

## Resumption Record: standalone cast-result authority contract blocker

Last accepted progress: none. No runbook step has been accepted, no accepted
implementation exists, and there are no accepted proof or commit references.

Interrupted step: Step 1 — `Publish the logical RHS conversion result`.

Interruption: an unaccepted local change in
`src/codegen/lir/hir_to_lir/expr/binary.cpp` replaces the logical integer RHS
`zext` result allocation with `fresh_value(ctx)`. It builds, but the focused
test still has its pre-Step-2 assertion that the RHS result lacks authority and
therefore fails with `FAIL: logical RHS result, PHI result, and later consumer
lack LirValueId authority`. This diff is not accepted work and must be cleaned
up or otherwise dispositioned by the supervisor before a clean reattempt; this
source does not accept it by record alone.

Blocker: the existing verifier/IR result-ownership collection considers only
present `result.value_id()` values, and `verify_cast_op_authority` permits an
absent result ID. Consequently it cannot make missing, invalid, duplicate, and
foreign standalone `LirCastOp` results fail closed. Raw PHI incoming entries
remain intentionally outside this source and cannot establish that contract.
The required verifier/IR authority contract is owned by
`ideas/open/779_lir_cast_result_authority_contract.md`, not by this logical
RHS producer source.

Exact return point after 779 closes: restart at Step 1, `Publish the logical
RHS conversion result`, with a clean reattempt of the producer change; then
perform Step 2 against the accepted standalone-cast verifier contract and
complete the Step 3 handoff. Remaining action is not a continuation of the
unaccepted diff. PHI result/incoming and generic logical migration remain
excluded.
