# RV64 Object Lowering For Prepared Control-Flow Fragments

Status: Open
Activation Priority: Deferred until the supervisor selects the RV64 object-lowering lane.
Type: Downstream backend follow-up
Parent: `ideas/open/560_bir_scalar_signature_control_semantic_producer_admission.md`
Owning Layer: RV64 object lowering

## Goal

Teach RV64 object lowering to handle the prepared BIR control-flow fragments
now exposed after scalar-control-flow BIR admission was repaired.

## Why This Exists

The scalar-control-flow lifecycle advanced its tracked representatives beyond
their original BIR `scalar-control-flow semantic family` admission diagnostics.
Several rows now fail later in RV64 object lowering, including prepared
terminator fragments, prepared move-bundle/select publication shapes, and BIR
`SelectInst` object fragments. Those failures are downstream from BIR
producer admission and should not keep the scalar-control-flow producer
runbook active.

## In Scope

- Repair RV64 object lowering for prepared BIR terminator fragments exposed by
  rows such as `src/20000314-3.c`, `src/930614-1.c`, and `src/pr35456.c`.
- Repair prepared move-bundle or select-publication object lowering exposed by
  rows such as `src/980604-1.c` and `src/pr39501.c`.
- Add focused backend coverage that proves the RV64 object-lowering behavior
  directly, not only through the torture progress harness.
- Re-run the relevant RV64 representatives and record any remaining downstream
  owner boundaries.

## Out Of Scope

- BIR scalar-control-flow CFG, terminator, or phi producer admission.
- Function-signature producer repair.
- Scalar-binop producer repair.
- Scalar/local-memory producer or lowering work.
- Expectation rewrites, unsupported downgrades, allowlist-only changes, or
  named torture-case shortcuts.

## Acceptance Criteria

- Focused RV64/backend coverage exists for the prepared control-flow object
  fragments being repaired.
- The relevant scalar-control-flow representatives advance beyond the current
  RV64 object-lowering diagnostics because the object-lowering capability is
  implemented.
- Any remaining failures are recorded as separate owner-boundary follow-ups
  instead of expanding this idea into unrelated producer work.

## Reviewer Reject Signals

- Reject changes that reclassify the rows, edit expectations, mark rows
  unsupported, or alter allowlists while the same RV64 object-lowering
  fragment still fails.
- Reject named-case handling for only `src/20000314-3.c`,
  `src/930614-1.c`, `src/pr35456.c`, `src/980604-1.c`, or
  `src/pr39501.c` instead of a semantic RV64 lowering rule for the fragment
  shape.
- Reject routing the issue back to BIR scalar-control-flow admission unless
  focused BIR evidence shows the prepared facts are malformed.
- Reject broad RV64 rewrites that do not add focused proof for the prepared
  terminator/select/object fragments.
- Reject claims of scalar-control-flow producer progress from this idea; its
  owner boundary is RV64 object lowering.
