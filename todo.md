Status: Active
Source Idea Path: ideas/open/564_bir_scalar_local_memory_semantic_admission_followup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Lock Down Current Scalar/Local-Memory Diagnostics

# Current Packet

## Just Finished

Activation created the scalar/local-memory semantic admission follow-up runbook
from `ideas/open/564_bir_scalar_local_memory_semantic_admission_followup.md`.

## Suggested Next

Execute Step 1 from `plan.md`: reproduce or inspect the current IEEE-row
`scalar/local-memory semantic family` diagnostics and map them to the concrete
BIR scalar/local-memory producer boundary before implementation.

## Watchouts

- Keep scalar-control-flow, function-signature, scalar-binop, and RV64
  object-lowering work out of this runbook unless focused diagnostics prove
  the owner boundary has changed.
- Do not use expectation rewrites, unsupported markers, allowlist edits,
  row reclassification, or named IEEE-row shortcuts as progress.
- Record the row-to-boundary mapping here before broadening the route.

## Proof

Lifecycle activation only; no build or test proof required.
