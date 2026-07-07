Status: Active
Source Idea Path: ideas/open/560_bir_scalar_signature_control_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Add Scalar-Control-Flow BIR Coverage

# Current Packet

## Just Finished

Lifecycle split completed after the prior Step 1 inspection proved that
function-signature, scalar-control-flow, and scalar-binop failures do not share
one repairable BIR semantic producer boundary. The active runbook now owns only
the scalar-control-flow producer route.

## Suggested Next

Delegate Step 1: Add Scalar-Control-Flow BIR Coverage.

## Watchouts

- Keep function-signature work in
  `ideas/open/561_bir_function_signature_semantic_producer_admission.md`.
- Keep scalar-binop work in
  `ideas/open/562_bir_scalar_binop_semantic_producer_admission.md`.
- Do not claim scalar-control-flow progress through expectation rewrites,
  unsupported downgrades, allowlist edits, or named-case shortcuts.

## Proof

Lifecycle-only split; no build or test proof required.
