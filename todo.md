Status: Active
Source Idea Path: ideas/open/563_rv64_object_lowering_control_flow_fragments.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Lock Down Current RV64 Object Fragment Failures

# Current Packet

## Just Finished

Lifecycle activation created this scratchpad for Step 1. No executor packet has
run yet.

## Suggested Next

Delegate Step 1 from `plan.md`: add or extend focused RV64/backend coverage for
the prepared terminator, prepared move-bundle/select-publication, and
`SelectInst` object-fragment failures exposed by the scalar-control-flow
representatives.

## Watchouts

- Keep BIR scalar-control-flow producer work in
  `ideas/open/560_bir_scalar_signature_control_semantic_producer_admission.md`.
- Keep function-signature, scalar-binop, and scalar/local-memory work in their
  own open ideas.
- Do not use row reclassification, expectation rewrites, unsupported markers,
  allowlist edits, or named torture-case shortcuts as progress.
- If focused BIR evidence shows prepared facts are malformed, stop and report
  the owner-boundary mismatch instead of expanding this RV64 object runbook.

## Proof

Not run. This was a lifecycle-only activation.
