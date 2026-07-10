Status: Active
Source Idea Path: ideas/open/671_prepared_destination_dump_contract_review.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Destination Dump Evidence

# Current Packet

## Just Finished

Step 1 `Refresh Destination Dump Evidence` refreshed the focused prepared-BIR
dump boundary for the three residual prepared destination rows.

The delegated proof command exited `8`: the build was up to date, and all
three focused dump tests failed with `[BACKEND_DUMP_SNIPPET_MISSING]`. Current
prepared facts are present for each row, so the current owner classification is
stale snippet expectation, not dump text emission and not missing prepared
publication:

- `backend_dump_riscv64_scalar_compare_frame_slot_destination`: stale snippet
  expectation. The missing snippet expects `home %t2 value_id=2 kind=stack_slot
  slot_id=5 offset=24`, while the current dump publishes `home %t2 value_id=3
  kind=stack_slot slot_id=5 offset=24`; the storage plan also publishes
  `storage %t2 value_id=3 encoding=frame_slot ... stack_offset=24`, and
  before-instruction moves target `to_value_id=3`.
- `backend_dump_riscv64_prepared_fused_compare_call_result_predicate`: stale
  snippet expectation. The missing snippet expects call-result
  `destination_value_id=0`, while the current prepared call plan publishes
  `destination_value_id=1` with `source_reg=a0`, `dest_reg=t0`, and the current
  value locations/storage plan publish `%t0 value_id=1`.
- `backend_dump_riscv64_function_pointer_return_chain`: stale snippet
  expectation. The missing snippet expects `home @sub value_id=5 kind=register
  reg=t0`, while the current dump publishes `home @sub value_id=6 kind=register
  reg=t0`; the storage plan also publishes `storage @sub value_id=6
  encoding=register ... reg=t0`.

## Suggested Next

Execute Step 2 by updating only the stale expected dump snippets for the three
focused rows, using the refreshed prepared facts above as the contract
authority.

## Watchouts

- Treat text-only expectation updates as dump-contract alignment, not compiler
  capability progress.
- All three rows currently have prepared facts in the dump; do not route Step 2
  into publication or printer code unless new evidence contradicts this log.
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

Result: exit `8`; build was up to date; all three focused tests failed with
`[BACKEND_DUMP_SNIPPET_MISSING]`. This is sufficient Step 1 evidence because
the failure output contains the current prepared facts needed to select the
Step 2 owner.

Test subset:

- `backend_dump_riscv64_scalar_compare_frame_slot_destination`
- `backend_dump_riscv64_prepared_fused_compare_call_result_predicate`
- `backend_dump_riscv64_function_pointer_return_chain`

Log path: `test_after.log`.
