# Current Packet

Status: Active
Source Idea Path: ideas/open/770_lir_to_bir_native_label_address_constant_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement direct-constant ordinary pointer-store consumption

## Just Finished

- Step 2 completed: `LirStoreOp.val` now admits only the structured,
  function-owned direct label-address pointer constant and renders it as
  `blockaddress(...)`; function value ownership registers that constant as a
  non-instruction definition and rejects malformed store authority. Focused
  positive rendering plus missing, duplicate, invalid/foreign owner or target,
  non-pointer/wrong-type, and mismatched-use checks passed without changing
  the accepted indirect-jump route.

## Suggested Next

- Supervisor: complete Step 3 acceptance handling and return 768 exactly to
  its interrupted Step 5; do not claim producer, table-decay, external, or
  broader backend integration proof.

## Watchouts

- The positive store fixture is deliberately LIR verifier/printer coverage;
  Raw-BIR import remains out of scope and the existing direct indirect-jump
  fixture continues to prove `e8a0f70b4`. No `LabelAddrExpr` producer,
  carrier, table-decay, 767/769, broad backend, or synthetic
  `select`/`gep`/`bitcast` bridge work belongs here.

## Proof

- Passed: `cmake --build --preset default` followed by
  `ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_interface$'`
  with output retained in `test_after.log`. This is sufficient for the bounded
  LIR store consumer and retained indirect-jump fixture, not 768 producer or
  external integration proof.
