# Cursor-Complete Prepared Call Plan Production

Status: Open
Type: common prepared-call producer repair
Parent: `ideas/open/703_bir_mir_contract_abstraction_umbrella.md`
Blocks: `ideas/open/708_x86_named_handoff_materializer_cleanup.md`

## Goal

Make common preparation publish exactly one cursor-exact `PreparedCallPlan`
for every supported semantic callsite, including zero-argument fixed-arity
calls adjacent to variadic calls, so target consumers can validate callee and
cursor identity without route-derived fallback.

## Why This Exists

The supported x86 direct-extern fixture contains semantic calls at instruction
cursors 0 and 1. Preparation currently publishes only the later `printf` call
and keys it at cursor 0. A consumer asking for the first call therefore sees a
plan with mismatched callee identity and must reject. This is a common producer
completeness defect, not authority that x86 should reconstruct.

## In Scope

- Trace semantic call enumeration through common prepared-call production.
- Repair filtering or cursor accounting so every supported callsite produces
  one plan keyed by its actual block and instruction cursor.
- Preserve callee, argument, result, ABI, move-bundle, and preservation
  identity for each produced plan.
- Add direct producer proof for adjacent mixed-shape calls, including a
  zero-argument fixed-arity call before a variadic call.
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
  true block/instruction cursor.
- The direct-extern mixed-call fixture exposes distinct plans for cursors 0
  and 1 with the correct callee and argument identities.
- Missing, duplicate, stale, or mismatched plans remain unavailable or
  rejected rather than being selected by position or source order.
- Producer-focused tests and the affected x86 handoff boundary test are green
  without expectation changes.
- A broader supervisor-selected backend regression comparison is green before
  the initiative is closed.

## Reviewer Reject Signals

- A named-fixture, callee-name, `printf`, or two-call special case is added
  instead of repairing semantic call enumeration and cursor identity.
- A consumer accepts the nearest, first, or source-order plan when exact
  block/cursor/callee identity is absent or inconsistent.
- Tests are downgraded to unsupported, assertions are weakened, or expected
  output is rewritten to hide a missing first-call plan.
- Helper renames, lookup reclassification, or diagnostic-only changes are
  claimed as producer capability progress while the fixture still publishes
  one plan at the wrong cursor.
- The change broadly rewrites target materializers, ABI policy, or unrelated
  preparation pipelines.
- Route-derived call authority survives behind a new common or prepared helper
  name.
