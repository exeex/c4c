# Current Packet

Status: Active
Source Idea Path: ideas/open/782_lir_vaarg_phi_input_result_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Publish native vaarg PHI-helper input fields

## Just Finished

- Plan Step 1 native vaarg helper-input producer fields: AArch64 GP/FP marks
  `reg_addr` GEP and `stack_ptr` load; FP also marks its `aligned_stack_ptr`
  ptrmask call or IntToPtr cast; AMD64 marks register `reg_value` and overflow
  `stack_value` loads. `LirGepOp`, `LirLoadOp`, and `LirCallOp` now have
  opt-in native-result requirements with missing/invalid/duplicate/foreign
  rejection coverage, without changing PHI carriers or PHI verification.

## Suggested Next

- Review the completed Step 1 slice for acceptance and select the next
  bounded Plan Step 1 packet only if remaining helper-input authority is found.

## Watchouts

- Treat 783's accepted source-to-immediate-consumer contracts as upstream
  authority, not as a 782 helper-field or PHI-completion claim.
- `LirVaArgOp.result` is a later result and does not identify helper PHI inputs.
- Do not change `LirPhiOp`, PHI verification, predecessor/edge authority, CFG,
  Raw-BIR/importer, backend, target lowering, MIR, or emission.
- Do not recover IDs from names, labels, rendered text, instruction order, or
  testcase text; do not introduce side tables or result-name maps.
- The FP alignment route uses the existing cast opt-in for the <=8-byte path;
  the new producer opt-ins remain restricted to GEP/load/call.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$'`. Per packet scope, no
  canonical root log was written.
