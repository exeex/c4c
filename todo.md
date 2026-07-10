Status: Active
Source Idea Path: ideas/open/671_prepared_destination_dump_contract_review.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Destination Dump And Nearby Regression Safety

# Current Packet

## Just Finished

Step 3 `Prove Destination Dump And Nearby Regression Safety` ran the
supervisor-selected focused prepared-destination dump plus nearby route/runtime
proof. The six selected tests all passed.

## Suggested Next

Supervisor should review the Step 3 proof result and decide whether the active
idea is ready for lifecycle close review or needs any additional non-overfit
coverage.

## Watchouts

- The three focused dump rows and three nearby route/runtime rows pass with
  current prepared facts; do not route follow-up work into publication or
  printer code unless new evidence contradicts this proof.
- Do not work on stack-passed parameter-home publication or caller ABI
  stack-binding; that is idea 672.
- Do not reopen ideas 647 or 655 stack-destination fan-in authority.
- Do not use testcase names, fixed value IDs, or final assembly shape as the
  dump-contract authority.

## Proof

Command:

```sh
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_dump_riscv64_scalar_compare_frame_slot_destination|backend_dump_riscv64_prepared_fused_compare_call_result_predicate|backend_dump_riscv64_function_pointer_return_chain|backend_codegen_route_riscv64_prepared_fused_compare_call_result_predicate|backend_codegen_route_riscv64_function_pointer_return_chain|backend_rv64_runtime_riscv64_function_pointer_return_chain)$') > test_after.log 2>&1
```

Result: exit `0`; build was up to date; all six selected tests passed.
This is sufficient for the delegated Step 3 regression-safety packet because it
covers the three updated dump contracts and the supervisor-selected nearby
route/runtime rows.

Test subset:

- `backend_dump_riscv64_scalar_compare_frame_slot_destination`
- `backend_dump_riscv64_prepared_fused_compare_call_result_predicate`
- `backend_dump_riscv64_function_pointer_return_chain`
- `backend_codegen_route_riscv64_prepared_fused_compare_call_result_predicate`
- `backend_codegen_route_riscv64_function_pointer_return_chain`
- `backend_rv64_runtime_riscv64_function_pointer_return_chain`

Log path: `test_after.log`.
