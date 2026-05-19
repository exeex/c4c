Status: Active
Source Idea Path: ideas/open/314_aarch64_large_stack_offset_addressing.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Large Stack Offset Addressing Paths

# Current Packet

## Just Finished

Step 1 localization complete for idea 314. The `00216.c` residual reaches
emitted AArch64 assembly through the normal prepared memory load/store path:
`memory.cpp` builds `MemoryOperand{base_kind=FrameSlot, byte_offset=...,
can_use_base_plus_offset=true}`, `machine_printer.cpp::print_memory` prints
`memory_address(...)` directly as `[sp, #offset]`, and the assembler rejects
the representative `ldr x13, [sp, #1644]`. The `00204.c` residual reaches the
scalar ALU publication path: `alu.cpp::make_prepared_scalar_result_operand`
records `result_stack_offset_bytes`, `lower_scalar_instruction` carries it into
`ScalarAluRecord`, and `scalar_alu_stack_publication_line` rejects publication
offsets outside `0..4095` with `scalar ALU stack publication offset is not
printable`.

Conclusion: these residuals share one repair owner, legal materialization for
large prepared frame-slot offsets, but should be implemented as ordered
substeps because the entry points differ. First repair/cover the generic
`MemoryOperand` frame-slot load/store printer path (`memory_address`,
`print_memory`, stack-source store helpers). Then reuse the same materialization
strategy for scalar ALU result publication in `alu.cpp` instead of keeping a
separate direct `str result, [sp, #offset]` range check.

## Suggested Next

Execute Step 2 as a code packet. Add focused backend coverage first in the
existing local surfaces, then implement the large frame-slot offset
materialization helper and route both direct memory load/store printing and
scalar ALU stack publication through it.

## Watchouts

- This owner is large stack-offset addressing/publication work, not semantic
  local-memory prepared-handoff work from closed idea 312 or f128 transport
  work from closed idea 313.
- Owning code surfaces localized for the next packet:
  `src/backend/mir/aarch64/codegen/memory.cpp` (`memory_address` currently
  spells direct `[sp, #offset]`), `machine_printer.cpp` (`print_memory`,
  stack-source store scratch paths, and existing symbol/local-address helpers),
  and `alu.cpp` (`scalar_alu_stack_publication_line`,
  `append_scalar_alu_stack_publication`, `make_prepared_scalar_result_operand`,
  `lower_scalar_instruction`).
- Representative local tests are `backend_aarch64_machine_printer` for direct
  frame-slot load/store printing and scalar ALU publication text,
  `backend_aarch64_target_instruction_records` for structured record facts, and
  `backend_aarch64_instruction_dispatch` for selected prepared-memory/scalar
  records. Representative external tests are
  `c_testsuite_aarch64_backend_src_00216_c` and
  `c_testsuite_aarch64_backend_src_00204_c`.
- The smallest focused proof command for this owner is:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)|backend_cli_dump_(bir|prepared_bir)_(00204_stdarg_(semantic|prepared)_handoff|focus_(function_filters|block_entry)_00204)|c_testsuite_aarch64_backend_src_002(04|16)_c)$'`
- Keep umbrella idea 295 parked; do not fold unrelated runtime, timeout,
  direct-call, vararg, address-of-local, `00164.c`, or `00214.c` residuals
  into this owner.
- Do not change expectations, unsupported classifications, allowlists, runner
  behavior, timeout policy, proof-log policy, CTest registration, semantic
  handoff contracts, or f128 transport behavior.
- Do not satisfy the route with filename, function-name, offset-literal,
  stack-slot-id, scalar-opcode, diagnostic-string, or c-testsuite-number
  special cases.

## Proof

Ran:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)|backend_cli_dump_(bir|prepared_bir)_(00204_stdarg_(semantic|prepared)_handoff|focus_(function_filters|block_entry)_00204)|c_testsuite_aarch64_backend_src_002(04|16)_c)$'`

Result recorded in `test_after.log`: build succeeded; CTest ran 12 tests with
10 passing and the two expected large-offset residuals failing:
`c_testsuite_aarch64_backend_src_00216_c` at `ldr x13, [sp, #1644]` and
`c_testsuite_aarch64_backend_src_00204_c` at scalar ALU stack publication
offset printing.
