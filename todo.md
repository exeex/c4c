# Current Packet

Status: Active
Source Idea Path: ideas/open/782_lir_vaarg_phi_input_result_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Publish native vaarg PHI-helper input fields

## Just Finished

- Plan Step 1 repaired AMD64 overflow helper authority: the intermediate
  `stack_ptr` pointer load no longer opts in, and the returned final
  `stack_value` load does. Focused AMD64 coverage identifies the selected
  non-pointer native producer structurally without inspecting `LirPhiOp`.

## Suggested Next

- Review the repaired Step 1 slice for acceptance and select the next bounded
  Step 1 packet only if another helper-input authority defect is found.

## Watchouts

- Treat 783's accepted source-to-immediate-consumer contracts as upstream
  authority, not as a 782 helper-field or PHI-completion claim.
- `LirVaArgOp.result` is a later result and does not identify helper PHI inputs.
- AMD64 overflow authority belongs to the final non-pointer result load, not
  the pointer load used as the memcpy source.
- Do not change `LirPhiOp`, PHI verification, predecessor/edge authority, CFG,
  Raw-BIR/importer, backend, target lowering, MIR, or emission.
- Do not recover IDs from names, labels, rendered text, instruction order, or
  testcase text; do not introduce side tables or result-name maps.
- The FP alignment route uses the existing cast opt-in for the <=8-byte path;
  the new producer opt-ins remain restricted to GEP/load/call.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$'`. Per delegated scope,
  no canonical root log was written.
