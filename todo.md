Status: Active
Source Idea Path: ideas/open/313_aarch64_f128_transport_machine_printer.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair F128 Transport Printing Or Lowering

# Current Packet

## Just Finished

Step 2 repaired the selected-but-unprintable `f128_transport` memory-backed
carrier path without testcase-specific matching. `print_f128_transport` now
spells memory-backed f128 load/store transports through the target reserved
FP/SIMD scratch q-register (`q16`): load memory to scratch and store scratch to
the carrier frame slot for `LoadFromMemory`, or load the carrier frame slot to
scratch and store scratch to memory for `StoreToMemory`. Incomplete
memory-backed records still fail closed when carrier frame-slot authority or
16-byte memory facts are missing. The machine-effect contract now also marks
memory-backed stores as using the prepared f128 carrier value, matching the
scratch load from the carrier slot that the printer emits.

Focused tests now cover the memory-backed record contract in
`backend_aarch64_target_instruction_records` and the scratch-printing plus
incomplete-slot fail-closed behavior in `backend_aarch64_machine_printer`.
The old `00204.c` first-bad `f128_transport` printer diagnostic is gone.

## Suggested Next

Localize the new `00204.c` first-bad AArch64 printer residual:
`function 0 block 6 instruction 0: cannot print AArch64 machine node
family=scalar opcode=add: scalar ALU stack publication offset is not
printable`. Determine whether it belongs to idea 313's f128 transport owner or
should be handed to a separate scalar stack-publication/large-offset owner.

## Watchouts

- This owner is target machine-node printer/lowering work, not semantic
  local-memory prepared-handoff work from closed idea 312.
- Keep `ideas/open/314_aarch64_large_stack_offset_addressing.md` parked; do
  not fold the `00216.c` large stack-offset residual into this owner.
- The new scalar stack-publication failure may be related to large stack
  offsets, but this packet did not inspect or repair it.
- Do not match on `00204.c`, `stdarg`, function/block/instruction numbers, or
  diagnostic strings.
- Do not change expectations, unsupported classifications, allowlists, runner
  behavior, timeout policy, proof-log policy, CTest registration, or semantic
  handoff contracts.

## Proof

Ran:
`cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|backend_lir_to_bir_notes|backend_cli_dump_bir_00204_stdarg_semantic_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff|backend_cli_dump_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_block_entry_00204|c_testsuite_aarch64_backend_src_00204_c)$' > test_after.log 2>&1`

Result: build succeeded. CTest ran 10 tests; 9 passed, including
`backend_aarch64_machine_printer`, `backend_aarch64_instruction_dispatch`, and
`backend_aarch64_target_instruction_records`. `c_testsuite_aarch64_backend_src_00204_c`
still fails, but it advanced past the old `f128_transport` diagnostic to:
`cannot print AArch64 machine node family=scalar opcode=add: scalar ALU stack
publication offset is not printable`. Proof log: `test_after.log`.
