# Current Packet

Status: Active
Source Idea Path: ideas/open/773_lir_gep_direct_label_address_constant_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Specify and verify the typed direct-label-address GEP base

## Just Finished

- Step 1 completed: `LirGepOp.ptr` now admits only a typed
  `DirectConstant(LirValueId)` at the GEP-specific gate and resolves it in
  `verify_function_value_ownership` against the current function's
  pointer-typed direct-label-address table. Nearby interface coverage proves
  the positive verifier path and rejects missing, arbitrary, non-pointer,
  display-inconsistent, and non-pointer-form bases.

## Suggested Next

- Supervisor: select the next 773 packet; keep any printer/backend receipt
  work separate from this verifier-only Step 1 slice.

## Watchouts

- Generic pointer validation remains unchanged for all non-GEP operands and
  non-direct GEP bases. Direct GEP identity is table-backed only; no label
  spelling or printer text is authority.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build
  --output-on-failure -R '^backend_lir_to_bir_interface$' | tee test_after.log`.
  The fresh build and focused interface subset passed; proof output is
  `test_after.log`.
