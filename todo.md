# Current Packet

Status: Active
Source Idea Path: ideas/open/782_lir_vaarg_phi_input_result_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Publish native vaarg PHI-helper input fields

## Just Finished

- Closed 783 capability-complete and resumed 782 at its preserved Step 1; no
  782 implementation packet or after-proof has been accepted.

## Suggested Next

- Execute Plan Step 1: trace the native defining operations for every raw input
  to the AArch64 GP, AArch64 FP, and AMD64 vaarg PHI constructors, then publish
  only the bounded helper-input fields.

## Watchouts

- Treat 783's accepted source-to-immediate-consumer contracts as upstream
  authority, not as a 782 helper-field or PHI-completion claim.
- `LirVaArgOp.result` is a later result and does not identify helper PHI inputs.
- Do not change `LirPhiOp`, PHI verification, predecessor/edge authority, CFG,
  Raw-BIR/importer, backend, target lowering, MIR, or emission.
- Do not recover IDs from names, labels, rendered text, instruction order, or
  testcase text; do not introduce side tables or result-name maps.

## Proof

- Plan Step 2 requires a fresh build and focused three-constructor vaarg
  result-authority proof. The supervisor selects any broader acceptance proof.
