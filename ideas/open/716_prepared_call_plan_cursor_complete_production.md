# Cursor-Complete Prepared Call Plan Production

Status: Open
Type: common prepared-call producer repair
Parent: `ideas/open/703_bir_mir_contract_abstraction_umbrella.md`
Blocks: `ideas/open/708_x86_named_handoff_materializer_cleanup.md`

## Goal

Make common preparation publish exactly one cursor-exact `PreparedCallPlan`
for every supported semantic callsite by treating each `CallInst` operand as
the base argument identity and optional source-relationship metadata as a
refinement, so target consumers can validate complete callee, argument, and
cursor identity without route-derived fallback.

## Why This Exists

The supported x86 direct-extern fixture contains semantic calls at instruction
cursors 0 and 1. Preparation already publishes the zero-argument
`actual_function` call at its exact cursor 0. It omits the argument-bearing
`printf` call at cursor 1 because `find_call_argument` reports `Incomplete`
when the semantic operand has no optional `CallArgumentSourceRelationship`,
and `populate_call_plans` then drops the whole call. This is a common argument
identity/completeness defect, not a cursor-assignment defect and not authority
that x86 should reconstruct.

## In Scope

- Treat `CallInst::args[arg_index]` as the base identity for every semantic
  argument during common prepared-call production.
- Allow one unique optional `CallArgumentSourceRelationship` to refine source,
  selection, aggregate-lane, and producer identity without making that row
  mandatory.
- Keep ambiguous, out-of-range, stale, or semantic-operand-contradicting
  relationship evidence fail closed rather than selecting a row by order.
- Preserve the already-correct semantic instruction cursor for every emitted
  plan; do not renumber calls by surviving plan-vector position.
- Preserve callee, argument, result, ABI, move-bundle, and preservation
  identity for each produced plan.
- Add direct producer proof for adjacent mixed-shape calls, including the
  cursor-0 zero-argument fixed-arity call and cursor-1 argument-bearing
  variadic call, plus nearby argument-bearing direct-BIR shapes without
  optional source relationships.
- Add negative proof for missing, duplicate, stale-cursor, and callee/cursor
  mismatch states at the prepared lookup/consumer boundary.

## Out Of Scope

- Do not modify x86, AArch64, or RV64 materializers.
- Do not restore Route 6 or introduce target-local call-plan synthesis.
- Do not change ABI classification or supported call semantics.
- Do not weaken fail-closed lookup behavior or existing handoff expectations.
- Do not absorb memory, publication, joined-control, debug-vocabulary, or BIR
  route-quarantine work.

## Acceptance Criteria

- Each supported semantic callsite has exactly one `PreparedCallPlan` at its
  true block/instruction cursor, whether its semantic arguments have no source
  relationship or one unique compatible refinement.
- The direct-extern mixed-call fixture exposes distinct plans for cursors 0
  and 1 with the correct callee and argument identities.
- Ambiguous, stale, out-of-range, or operand-contradicting relationship
  evidence rejects the affected plan rather than erasing semantic identity or
  choosing an arbitrary refinement.
- Missing, duplicate, stale, or mismatched plans remain unavailable or
  rejected rather than being selected by position or source order.
- Producer-focused tests and the affected x86 handoff boundary test are green
  without expectation changes.
- A broader supervisor-selected backend regression comparison is green before
  the initiative is closed.

## Reviewer Reject Signals

- A named-fixture, callee-name, `printf`, zero-argument, or two-call special
  case is added instead of repairing semantic operand/refinement handling.
- A consumer accepts the nearest, first, or source-order plan when exact
  block/cursor/callee identity is absent or inconsistent.
- Tests are downgraded to unsupported, assertions are weakened, or expected
  output is rewritten to hide the missing argument-bearing cursor-1 plan.
- Helper renames, lookup reclassification, or diagnostic-only changes are
  claimed as producer capability progress while the argument-bearing cursor-1
  call is still omitted or relationship ambiguity no longer fails closed.
- The change broadly rewrites target materializers, ABI policy, or unrelated
  preparation pipelines.
- Route-derived call authority survives behind a new common or prepared helper
  name.
