Status: Active
Source Idea Path: ideas/open/672_stack_passed_parameter_home_dump_contract_split.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Focused And Nearby Regression Safety

# Current Packet

## Just Finished

Plan Step 3 proved the active stack-passed parameter-home dump contract plus
the supervisor-selected nearby prepared-destination regression surface.

Command:

```sh
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_dump_riscv64_stack_passed_parameter_home_publication|backend_dump_riscv64_scalar_compare_frame_slot_destination|backend_dump_riscv64_prepared_fused_compare_call_result_predicate|backend_dump_riscv64_function_pointer_return_chain|backend_codegen_route_riscv64_prepared_fused_compare_call_result_predicate|backend_codegen_route_riscv64_function_pointer_return_chain|backend_rv64_runtime_riscv64_function_pointer_return_chain)$') > test_after.log 2>&1
```

Result: exit `0`; build completed, and all seven selected tests passed:

- `backend_dump_riscv64_stack_passed_parameter_home_publication`
- `backend_dump_riscv64_scalar_compare_frame_slot_destination`
- `backend_dump_riscv64_prepared_fused_compare_call_result_predicate`
- `backend_dump_riscv64_function_pointer_return_chain`
- `backend_codegen_route_riscv64_prepared_fused_compare_call_result_predicate`
- `backend_codegen_route_riscv64_function_pointer_return_chain`
- `backend_rv64_runtime_riscv64_function_pointer_return_chain`

This proves the active dump contract plus nearby prepared-destination
route/runtime surface. It is not backend capability progress.

## Suggested Next

Ask the plan owner to decide whether the focused proof is sufficient to close,
retire, or split idea 672.

## Watchouts

- Keep ideas 647 and 655 parked unless new evidence proves their
  stack-destination fan-in authority scope is required.
- Do not update further expectations without current caller ABI and callee
  parameter-home evidence.
- Do not claim text-only expectation churn as backend capability progress.
- The current evidence supports an expectation-contract repair only; it does
  not prove new backend capability.

## Proof

Focused and nearby regression proof:

```sh
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_dump_riscv64_stack_passed_parameter_home_publication|backend_dump_riscv64_scalar_compare_frame_slot_destination|backend_dump_riscv64_prepared_fused_compare_call_result_predicate|backend_dump_riscv64_function_pointer_return_chain|backend_codegen_route_riscv64_prepared_fused_compare_call_result_predicate|backend_codegen_route_riscv64_function_pointer_return_chain|backend_rv64_runtime_riscv64_function_pointer_return_chain)$') > test_after.log 2>&1
```

Result: exit `0`; all seven selected tests passed. `test_after.log` preserved
for the passing proof.
