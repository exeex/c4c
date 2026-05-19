Status: Active
Source Idea Path: ideas/open/313_aarch64_f128_transport_machine_printer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize F128 Transport Printer Requirements

# Current Packet

## Just Finished

Step 1 localized the `00204.c` AArch64 `f128_transport` residual to an
internal contract mismatch between selection/lowering and the machine printer.
The failing proof reaches the machine-node printer, then rejects function 0
block 0 instruction 1 with:
`cannot print AArch64 machine node family=f128_transport opcode=f128_transport:
f128 transport printer requires structured full-width q-register authority`.

The missing structured fact is not a semantic local-memory handoff fact. It is
that a selected `f128_transport` node must carry printer-usable full-width
q-register authority, or memory-backed f128 carriers must have a coherent local
transport spelling before they are considered selected/printable. Today
`make_prepared_f128_carrier_transport_record` and
`validate_f128_transport_instruction` in
`src/backend/mir/aarch64/codegen/f128.cpp` admit
`PreparedF128CarrierKind::MemoryBacked` when frame-slot authority exists, while
`print_f128_transport` only prints `FullWidthRegister` records with a structured
q-register. `lower_f128_transport_instruction` in
`src/backend/mir/aarch64/codegen/memory.cpp` forwards the selected record to the
printer route, so the selected-but-unprintable memory-backed case escapes.

## Suggested Next

Implement the narrow Step 2 repair so `f128_transport` selection and printing
agree for memory-backed f128 carriers: either lower memory-backed carriers
through a structured q-register/scratch transport or fail closed before printer
selection instead of emitting a selected unprintable node.

Likely owning surfaces are `print_f128_transport`,
`validate_f128_transport_instruction`, and
`make_prepared_f128_carrier_transport_record` in
`src/backend/mir/aarch64/codegen/f128.cpp`, plus
`lower_f128_transport_instruction` in
`src/backend/mir/aarch64/codegen/memory.cpp` if the lowering route must choose a
different transport form. The smallest local proof surface is:
`backend_aarch64_target_instruction_records` for the memory-backed record and
selection contract, `backend_aarch64_machine_printer` for direct selected-node
printing/fail-closed behavior, and `backend_aarch64_instruction_dispatch` only
if dispatch/lowering behavior changes. Existing full-width q-register
transport tests already cover the currently printable path.

## Watchouts

- This owner is target machine-node printer/lowering work, not semantic
  local-memory prepared-handoff work from closed idea 312.
- Keep `ideas/open/314_aarch64_large_stack_offset_addressing.md` parked; do
  not fold the `00216.c` large stack-offset residual into this owner.
- The reported function/block/instruction location is only the observed first
  bad fact; do not match on it, on `00204.c`, on `stdarg`, or on the diagnostic
  string.
- The repair should preserve fail-closed behavior: no selected
  `f128_transport` node should be accepted if the printer cannot spell it from
  structured operands.
- Do not change expectations, unsupported classifications, allowlists, runner
  behavior, timeout policy, proof-log policy, CTest registration, or semantic
  handoff contracts.
- Do not satisfy the route with filename, function-name, block-number,
  instruction-number, diagnostic-string, or c-testsuite-number special cases.

## Proof

Ran:
`cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|backend_lir_to_bir_notes|backend_cli_dump_bir_00204_stdarg_semantic_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff|backend_cli_dump_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_block_entry_00204|c_testsuite_aarch64_backend_src_00204_c)$' > test_after.log 2>&1`

Result: build succeeded with no work to do. CTest ran 10 tests; 9 passed and
`c_testsuite_aarch64_backend_src_00204_c` failed with the localized
`f128_transport` machine-printer diagnostic above. Proof log:
`test_after.log`.
