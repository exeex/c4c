# Current Packet

Status: Active
Source Idea Path: ideas/open/782_lir_vaarg_phi_input_result_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Publish native vaarg PHI-helper input fields

## Just Finished

- Lifecycle switch: 751 Step 1 is parked pending this bounded vaarg helper-input
  identity prerequisite; no 751 implementation packet or after-proof was
  accepted.

## Suggested Next

- Begin Plan Step 1 by tracing the native defining operations for all raw inputs
  to the AArch64 GP, AArch64 FP, and AMD64 vaarg PHI constructors.

## Watchouts

- `LirVaArgOp.result` is a later result and does not identify helper PHI inputs.
- Do not change `LirPhiOp`, PHI verification, predecessor/edge authority,
  Raw-BIR/importer, backend, target lowering, MIR, or emission.
- Do not recover IDs from names, labels, rendered text, instruction order, or
  testcase text; do not introduce side tables or result-name maps.

## Proof

- Plan Step 2 requires a fresh build and focused three-constructor vaarg
  result-authority proof. The supervisor selects any broader acceptance proof.
