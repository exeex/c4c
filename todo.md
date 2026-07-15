Status: Active
Source Idea Path: ideas/open/800_lir_amd64_vaarg_unselected_alloca_compatibility_regression.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Restore typed alloca-result compatibility on unselected overflow routes

# Current Packet

## Just Finished

- Completed plan Step 1: the unselected AMD64 overflow `va_arg` temporary now
  uses `fresh_value(ctx)`, so its `LirAllocaOp` result is a typed SSA operand.
  The selected `fresh_value` path, destination/carrier construction, and
  native-memory authority boundary were left unchanged.

## Suggested Next

- Supervisor: execute plan Step 2's matching full baseline and compare it to
  the accepted 3037/3037 result before returning 753 to its receiver handoff.

## Watchouts

- This correction is limited to the unselected compatibility construction seam;
  do not weaken `LirAllocaOp` verification or alter selected-carrier authority
  when running the parent baseline gate.

## Proof

- Passed: `cmake --build --preset default` and
  `./build/tests/frontend/frontend_lir_call_type_ref_test`.
  Proof log: `test_after.log`.
