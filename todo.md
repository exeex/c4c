# Current Packet

Status: Active
Source Idea Path: ideas/open/804_lir_phi_incoming_producer_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair only the selected producer-side handoff

## Just Finished

- Step 2 completed: scalar integer unary minus now publishes its `sub` result
  with `fresh_value(ctx)`, so a ternary else incoming keeps the native
  current-function `LirValueId`; vector and floating unary-minus paths retain
  their prior display-only result behavior.  The clean typed-PHI interface
  test now verifies a native unary-minus incoming and rejects the same incoming
  when reduced to a display-only operand.

## Suggested Next

- Supervisor: decide the next active-plan packet after the selected producer
  handoff and focused proof are complete.

## Watchouts

- Do not infer an ID from `%tN` display text.  The repair deliberately leaves
  verifier, CFG, PHI schema, edge, and predecessor authority unchanged.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^llvm_gcc_c_torture_src_vrp_2_c$' > test_after.log`
  completed successfully: 1/1 passed.  The clean
  `build/tests/backend/bir/backend_lir_to_bir_interface_test` also passed
  after exercising the positive and malformed incoming-authority cases.
  Proof log: `test_after.log`.
