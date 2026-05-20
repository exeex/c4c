Status: Active
Source Idea Path: ideas/open/316_aarch64_frame_slot_layout_consistency.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Current Frame Layout Boundary

# Current Packet

## Just Finished

Step 1 localized the current `00182` local-array frame-layout boundary to
slot assignment. Semantic BIR for `main` still passes `ptr %lv.buf.0` to
`print_led`, and prepared stack-layout collection sees the scalarized buffer
family, but only the first byte object receives a frame slot:
`object #30 name=%lv.buf.0 size=1 address_exposed=yes requires_home_slot=yes`
through `object #159 name=%lv.buf.129 size=1 address_exposed=no
requires_home_slot=no`; `slot #30 object_id=30 offset=0 size=1`; and
`prepared.func @main frame_size=1`. Downstream AArch64 emission follows that
contract and emits `main` with `sub sp, sp, #48`, saved LR at `[sp, #24]`, and
passes `add x1, sp, #0` to `print_led`, so the callee writes through a pointer
whose prepared frame contract owns only one byte.

## Suggested Next

Execute Step 2/3 as one narrow packet if the supervisor agrees: add focused
prepared-layout coverage for an address-taken scalarized local aggregate whose
base address is passed to a call, prove the full contiguous aggregate extent is
assigned frame slots and contributes to `frame_size`, then repair the shared
slot-assignment/layout owner rather than AArch64 prologue emission.

## Watchouts

- Do not special-case `00182`, `print_led`, `MAX_DIGITS`, `buf`, `00216`, one
  stack offset, one function, or one instruction sequence.
- Do not reopen unsigned div/rem producer publication; idea 350 is parked
  unless fresh evidence shows stale div/rem consumers still exist.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, CTest registration, or proof-log behavior.
- Current `00216` is still red in the external runner, but the current
  frame-size evidence is not the same first bad fact: prepared `foo` reports
  `frame_size=1617`, saved registers at stack offsets 1624 and 1632, and the
  generated prologue allocates 1664 bytes. Treat `00216` as an external
  regression guard for now, not the Step 2 focused coverage source unless fresh
  artifacts expose a current out-of-frame slot.

## Proof

Step 1 proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_stack_layout|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|backend_aarch64_memory_operand_contract|c_testsuite_aarch64_backend_src_00182_c|c_testsuite_aarch64_backend_src_00216_c)$' | tee test_after.log`

Result: build was up to date; focused backend/prepared tests passed
(`backend_prepare_stack_layout`,
`backend_prepare_frame_stack_call_contract`,
`backend_cli_dump_prepared_bir_local_arg_call_contract`,
`backend_aarch64_memory_operand_contract`). External representatives
`c_testsuite_aarch64_backend_src_00182_c` and
`c_testsuite_aarch64_backend_src_00216_c` both still fail with
`RUNTIME_NONZERO` segmentation faults. Proof log: `test_after.log`.
