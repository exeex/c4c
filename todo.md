Status: Active
Source Idea Path: ideas/open/316_aarch64_frame_slot_layout_consistency.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Frame Layout Coverage and Repair Frame Size And Slot Consistency

# Current Packet

## Just Finished

Step 2/3 added focused prepared stack-layout coverage for a call-escaped
scalarized local family and repaired shared slot assignment so a fixed sliced
family pulls all sibling slice objects into contiguous fixed frame slots. The
new fixture would have observed the previous one-byte-frame failure: escaping
`lv.scalar.call.0` now assigns six fixed one-byte frame slots at offsets 0..5
and reports `frame_size=6`.

## Suggested Next

Execute Step 3/3 as a narrow packet: localize the new `00182` runtime mismatch
after the frame-underallocation repair and determine whether the next owner is
prepared call/address materialization, AArch64 call lowering, or another shared
prepared-frame contract.

## Watchouts

- Do not special-case `00182`, `print_led`, `MAX_DIGITS`, `buf`, `00216`, one
  stack offset, one function, or one instruction sequence.
- Do not reopen unsigned div/rem producer publication; idea 350 is parked
  unless fresh evidence shows stale div/rem consumers still exist.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, CTest registration, or proof-log behavior.
- `00182` advanced past the prior underallocated-frame segmentation fault, but
  now fails with `RUNTIME_MISMATCH`: expected the mixed digit display and
  produced seven `7` digits. Do not treat the Step 2 frame-layout owner as the
  remaining first bad fact without fresh evidence.
- `00216` is still red in the external runner with `RUNTIME_NONZERO`
  segmentation fault. Existing Step 1 evidence said its prepared frame size did
  not match the one-byte scalarized-buffer failure; keep it as an external
  guard unless fresh artifacts expose a current out-of-frame slot.

## Proof

Step 2/3 proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_stack_layout|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|backend_aarch64_memory_operand_contract|c_testsuite_aarch64_backend_src_00182_c|c_testsuite_aarch64_backend_src_00216_c)$' | tee test_after.log`

Result: build completed and focused backend/prepared tests passed
(`backend_prepare_stack_layout`,
`backend_prepare_frame_stack_call_contract`,
`backend_cli_dump_prepared_bir_local_arg_call_contract`,
`backend_aarch64_memory_operand_contract`). External representative
`c_testsuite_aarch64_backend_src_00182_c` now fails with `RUNTIME_MISMATCH`
instead of the prior segmentation fault; `c_testsuite_aarch64_backend_src_00216_c`
still fails with `RUNTIME_NONZERO` segmentation fault. Proof log:
`test_after.log`.

Supervisor broader guard after the shared stack-layout repair:

`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed 141/141.
