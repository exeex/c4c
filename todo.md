# Current Packet

Status: Active
Source Idea Path: ideas/open/751_lir_phi_incoming_value_and_predecessor_identity.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Publish and verify typed PHI incoming authority

## Just Finished

- Plan Step 1 completed: `LirPhiOp` incoming entries now carry typed native
  values and current-function predecessor `LirBlockId`s for ternary, logical,
  and AArch64/AMD64 vaarg producers; verifier rejects missing, unknown,
  cross-function, and edge-incoherent authority.

## Suggested Next

- Ask the supervisor/plan owner to evaluate Step 1 acceptance and select the
  next runbook packet; do not widen this carrier slice into Raw-BIR or backend
  consumers.

## Watchouts

- Do not recover identity from `%t` names, labels, rendered LLVM, instruction
  order, or testcase text.
- Display labels remain printer compatibility mirrors; semantic PHI authority
  is the incoming operand and predecessor block ID.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$'`; output preserved in
  `test_after.log`. This is the delegated focused proof; supervisor selects
  canonical regression logs and broader acceptance proof.
