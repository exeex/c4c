Status: Active
Source Idea Path: ideas/open/100_bir_runtime_intrinsic_placeholder_identity_contract.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Focused Contract Proof

# Current Packet

## Just Finished

Completed Step 4 by adding focused contract proof for the runtime placeholder identity helper and the affected prepared call-plan routes.

Tests:

- Added `verify_runtime_placeholder_identity_helper_contract()` in `backend_prealloc_call_boundary_classification_test.cpp` to prove `prepared_variadic_entry_helper_kind_for_call()` classifies an invalid-link `llvm.va_arg.i32` runtime placeholder while rejecting both a valid linked `llvm.va_arg.i32` direct call and an indirect `llvm.*` display call.
- Added `check_runtime_placeholder_aggregate_publication_call_plan_contract()` in `backend_prepare_liveness_test.cpp` to prove prepared call plans keep a valid linked `llvm.user_sink` direct call eligible for local aggregate address publication while rejecting both raw `llvm.va_start.p0` compatibility placeholders and structured `CallInst::intrinsic` placeholders.
- Left `backend_prepare_stack_layout_test.cpp` unchanged after confirming its stack-object surface is not the right hook for the final aggregate-publication flag.

## Suggested Next

Proceed to Step 5 with any remaining public contract/dump coverage review for the runtime placeholder identity work, or ask plan-owner to decide whether the active runbook is exhausted if Step 4 was the last required proof.

## Watchouts

- The aggregate-publication proof intentionally asserts the final `PreparedCallArgumentPlan::allows_local_aggregate_address_publication` flag through prepared call plans; stack-layout object flags can be affected by earlier address-use analysis and are not a focused hook for this contract.
- The direct-call eligibility case is ordered first in the fixture so regalloc keeps the local frame address materialization on the register-backed route required for the final publication flag.
- No implementation files were edited in this packet.

## Proof

Delegated proof:

```sh
cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R 'backend_(prepare_liveness|prepare_stack_layout|prealloc_call_boundary_classification)' >> test_after.log 2>&1
```

Result: passed. Proof log: `test_after.log`.
