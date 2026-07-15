# LIR PHI Producer Helper Result Identity

Status: Open
Type: bounded LIR helper-result authority blocker
Blocked Consumer: `ideas/open/751_lir_phi_incoming_value_and_predecessor_identity.md`
Builds On: closed `ideas/closed/744_lir_remaining_ordinary_value_identity_publication.md`

## Goal

Publish native current-function `LirValueId` result authority through the
ternary, logical short-circuit, and vaarg helper chains that must later supply
typed incoming values to `LirPhiOp`.

## Why This Exists

751's first-loss audit found no PHI-only implementation route. `LirPhiOp`
incoming entries are raw pairs, while its required producers lose result
identity before reaching that carrier: ternary routes use `emit_rval_id` /
`coerce`, logical routes use `fresh_tmp`, and vaarg helpers ultimately append
raw-string-result operations through `emit_lir_op`. Neither the existing raw
PHI pairs nor their display strings can create an authoritative value ID.

Closed 744 supplied generic ordinary `LirInst` result/use identity, but its
accepted matrix and handoff do not cover these helper-result chains. This is a
new bounded producer prerequisite, not a reopening of 744 or a PHI text
fallback.

## In Scope

- establish the first shared helper-result carrier seam that can allocate an
  owning current-function `LirValueId` before rendering
- migrate only the ternary/coerce, logical short-circuit, and vaarg helper
  result chains needed as PHI incoming-value producers
- ensure `emit_lir_op` receives structured result authority for those chains
  rather than a raw result string
- add focused producer-path and malformed ownership coverage for each named
  family, and publish an exact typed handoff to 751

## Out Of Scope

- `LirPhiOp` representation, PHI incoming verification, predecessor/edge
  checks, and Raw-BIR import; 751 and later 734 own those consumers
- reopening closed 744's ordinary-instruction matrix or modifying unrelated
  call, local/object, memory/va, aggregate/vector, target-lowering, MIR, or
  emission families
- deriving IDs from `%t` names, labels, printer/LLVM text, instruction order,
  or testcase identity
- parallel value tables, result-name maps, synthetic instructions, expectation
  downgrades, or testcase-specific branches

## Acceptance Criteria

- Each named helper producer allocates or forwards a valid owning
  current-function `LirValueId` before any display result is rendered.
- The relevant `emit_lir_op` path carries that structured result authority for
  the named chains; raw result spelling remains compatibility/display only.
- Focused positive coverage proves ternary, logical, and vaarg helper results
  can reach the eventual PHI input seam structurally; malformed missing,
  invalid, duplicate, and foreign results fail closed.
- The accepted handoff identifies the shared producer seam, typed field,
  covered families, proof, and exact 751 return point. No PHI carrier or
  Raw-BIR receiver work is claimed.

## Reviewer Reject Signals

- Reject parsing `%t` names, labels, rendered LLVM, printer output, or
  testcase text to populate a helper result ID.
- Reject a PHI-only side table, a result-name lookup, a synthetic SSA value, or
  a test-specific producer branch that leaves the raw helper chain unchanged.
- Reject a call-only or ordinary-`LirInst` rewrite claimed to cover ternary,
  logical, and vaarg helpers without focused evidence for all three.
- Reject changes to `LirPhiOp`, Raw-BIR/importer, CFG predecessor authority,
  target lowering, or any unrelated value family as progress for this blocker.
- Reject malformed-authority weakening, expectation downgrades, or baseline
  edits in place of native structured result publication.

## Resumption Record: typed expression-result carrier decomposition

No 775 implementation packet, proof, or failure-family reduction occurred.
The first-loss audit disproved the claimed shared helper seam: satisfying all
three families would require either a broad generic
`emit_rval_payload` / `emit_rval_id` / `coerce` API migration or PHI
carrier/verifier changes, both outside this source's bounded scope.

Classification: `separate decomposition initiative`. Open
`ideas/open/776_lir_typed_expression_result_carrier_decomposition.md` owns the
smaller carrier-boundary and one-family-at-a-time probes. It must not claim a
generic all-expression migration, PHI text recovery, side tables, or Raw-BIR
work as a shortcut.

Exact return point: after 776 accepts a typed expression-result handoff for a
single evidenced family and names the remaining unproven families, reactivate
775 only if its bounded helper contract is then executable without broad API
migration or PHI-carrier changes. Otherwise preserve 751's existing blocked
return and create a separately scoped successor from the accepted 776 facts.

## Accepted 778 Logical-RHS Producer Handoff

Closed `ideas/closed/778_lir_logical_rhs_result_authority_publication.md`
publishes only the logical RHS non-`i1` conversion producer fact: its
`LirCastOp.result` is a native module-unique, current-function-owned
`LirValueId` allocated before rendering, and it opts into the existing
standalone cast-result verifier. Focused positive coverage and missing,
invalid, same-function-duplicate, and foreign-authority rejection proof are
accepted in `3b716c12d`, `54ebfa4df`, and `b4685da80`; the fresh parent guard
`^(frontend_lir_call_type_ref|llvm_gcc_c_torture_src_pr52129_c)$` passed 2/2
with no changes and the supervisor accepted a fresh 3037/3037 full-suite
candidate into `test_baseline.log`. Module-wide ownership restoration was
accepted in closed 780 commits `7b9d6152b` and `3602e8fd2`.

775's exact return point is to consume this logical RHS cast-result fact only
when 775 is later activated and reassesses its bounded helper contract. Raw
logical PHI result/incoming entries and the final logical consumer remain
unresolved and excluded; this is not an accepted logical chain to the PHI
input seam. This handoff does not reactivate 775 or 751, alter 751, or
authorize generic expression API, PHI carrier/verifier, Raw-BIR/importer, or
backend work.
