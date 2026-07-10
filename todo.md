Status: Active
Source Idea Path: ideas/open/671_prepared_destination_dump_contract_review.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Select Repair Owner And Patch Narrowly

# Current Packet

## Just Finished

Step 2 `Select Repair Owner And Patch Narrowly` aligned only the three stale
prepared-destination dump snippets with the current Step 1 prepared facts:

- `backend_dump_riscv64_scalar_compare_frame_slot_destination`: updated the
  expected home fact from `%t2 value_id=2` to `%t2 value_id=3`.
- `backend_dump_riscv64_prepared_fused_compare_call_result_predicate`: updated
  the expected call result fact from `destination_value_id=0` to
  `destination_value_id=1`.
- `backend_dump_riscv64_function_pointer_return_chain`: updated the expected
  function-pointer home fact from `@sub value_id=5` to `@sub value_id=6`.

This is dump-contract alignment over already emitted prepared facts, not
lowering/compiler progress.

## Suggested Next

Run the Step 3 regression-safety packet for the focused prepared-destination
dump surface and any supervisor-selected nearby route/runtime coverage.

## Watchouts

- The three focused rows now pass with current prepared facts; do not route
  follow-up work into publication or printer code unless new evidence
  contradicts this proof.
- Do not work on stack-passed parameter-home publication or caller ABI
  stack-binding; that is idea 672.
- Do not reopen ideas 647 or 655 stack-destination fan-in authority.
- Do not use testcase names, fixed value IDs, or final assembly shape as the
  dump-contract authority.

## Proof

Command:

```sh
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_dump_riscv64_scalar_compare_frame_slot_destination|backend_dump_riscv64_prepared_fused_compare_call_result_predicate|backend_dump_riscv64_function_pointer_return_chain)$') > test_after.log 2>&1
```

Result: exit `0`; build was up to date; all three focused dump tests passed.
This is sufficient for the delegated Step 2 expectation-contract slice because
the proof covers exactly the three updated stale snippets.

Test subset:

- `backend_dump_riscv64_scalar_compare_frame_slot_destination`
- `backend_dump_riscv64_prepared_fused_compare_call_result_predicate`
- `backend_dump_riscv64_function_pointer_return_chain`

Log path: `test_after.log`.
