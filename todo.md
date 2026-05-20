Status: Active
Source Idea Path: ideas/open/316_aarch64_frame_slot_layout_consistency.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove External Representative And Reclassify Residuals

# Current Packet

## Just Finished

Step 4 proved that `00182` advanced past the old caller local-array
underallocation: prepared `main` now reports `frame_size=160` for
`%lv.buf.0..159`, and generated `main` reserves 192 bytes, passes `sp` to
`print_led`, and saves `x20/x21/x30` at `sp+160`, `sp+168`, and `sp+176`.
The new first bad fact is not frame-layout/slot consistency. Prepared
`print_led` keeps the digit extraction and lookup semantics intact
(`urem x, 10`, store to `d[n]`, then select `d[i]`), but generated AArch64
lowers each final digit lookup so the last `cmp i, #0` clobbers the accumulated
false select value and always passes `d[0]` to `topline`, `midline`, and
`botline`. Because `d[0] == 7` for `1234567`, the runtime output renders seven
`7` digits.

## Suggested Next

Split/escalate the residual out of idea 316 to an AArch64 scalar select/value
materialization owner. The smallest next packet should add focused coverage for
a nested select chain where the final select condition is false and the selected
value must come from the accumulated false operand rather than the first
element.

## Watchouts

- Do not special-case `00182`, `print_led`, `MAX_DIGITS`, `buf`, `00216`, one
  stack offset, one function, or one instruction sequence.
- Do not reopen unsigned div/rem producer publication; idea 350 is parked
  unless fresh evidence shows stale div/rem consumers still exist.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, CTest registration, or proof-log behavior.
- `00182` residual classification: AArch64 scalar select/value materialization.
  It is not frame-layout/slot consistency, prepared call/address
  materialization, AArch64 call lowering, or unsigned div/rem/digit semantics
  based on the current prepared/generated evidence.
- Concrete generated symptom: in `build/c_testsuite_aarch64_backend/src/00182.c.s`,
  each digit-render loop builds a select chain from `__static_local_print_led_3`,
  then ends with `cmp x13, #0; mov w9, w20; mov w21, w9; csel w21, w9, w21, eq`
  before calling the line renderer. Both csel operands now hold `d[0]`, so the
  accumulated `d[1..31]` false value is lost.
- `00216` remains red with `RUNTIME_NONZERO` segmentation fault, but current
  generated frame evidence still does not expose an idea-316 frame-layout
  residual: `foo` allocates 1664 bytes and the highest observed `sp` offset is
  1640; `test_correct_filling` allocates 144 bytes and `test_zero_init`
  allocates 96 bytes.

## Proof

Step 4 proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_stack_layout|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|backend_aarch64_memory_operand_contract|c_testsuite_aarch64_backend_src_00182_c|c_testsuite_aarch64_backend_src_00216_c)$' | tee test_after.log`

Result: build completed and focused backend/prepared tests passed
(`backend_prepare_stack_layout`,
`backend_prepare_frame_stack_call_contract`,
`backend_cli_dump_prepared_bir_local_arg_call_contract`,
`backend_aarch64_memory_operand_contract`). External representative
`c_testsuite_aarch64_backend_src_00182_c` fails with `RUNTIME_MISMATCH`
rendering seven `7` digits, and
`c_testsuite_aarch64_backend_src_00216_c` still fails with `RUNTIME_NONZERO`
segmentation fault. Because the command pipes through `tee`, the shell command
exited 0 while CTest reported 2 failed tests. Proof log: `test_after.log`.
