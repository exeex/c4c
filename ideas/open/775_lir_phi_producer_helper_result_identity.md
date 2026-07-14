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
