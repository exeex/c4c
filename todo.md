Status: Active
Source Idea Path: ideas/open/314_aarch64_large_stack_offset_addressing.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Large Stack Offset Materialization

# Current Packet

## Just Finished

Step 2 repair slice for idea 314 advanced the remaining call-boundary
large-offset case. In addition to the existing prepared memory load/store,
spill/reload, stack-source store, scalar memory-source load, and scalar ALU
stack publication materialization, `calls.cpp::print_call_boundary_move` now
materializes large prepared frame-slot source addresses before loading them
into call-boundary destination registers. Direct `[sp, #offset]` printing is
preserved for offsets encodable by the emitted load width; out-of-range or
unaligned offsets materialize `sp + offset` into a reserved scratch and use
`[scratch]`.

The original `00216.c` assembler failure at `ldr x13, [sp, #1644]` is gone.
`00216.c` now assembles/runs and fails with a runtime output mismatch. `00204.c`
still remains past scalar ALU stack publication and fails at the likely separate
frame setup immediate residual:
`frame adjustment immediate is outside the plain #imm encoding range 0..4095`.

## Suggested Next

Classify the new `00216.c` runtime mismatch and decide whether it belongs to
idea 314 or a separate runtime/initialization owner. Separately classify
`00204.c` frame setup immediate materialization as either an idea-314 follow-up
or a distinct frame-adjustment owner.

## Watchouts

- The local repaired paths are semantic width/range checks, not testcase-shaped
  matching: they key off frame-slot base, prepared offset, emitted load/store
  width, and reserved scratch availability.
- Call-boundary frame-slot source loads use the destination register view as
  the emitted load width; this avoids treating a 4-byte source-memory fact as
  encodable when the printer emits `ldr x...`.
- `00204.c` no longer reports `scalar ALU stack publication offset is not
  printable`; its new first bad fact is frame setup immediate materialization,
  which may be a separate owner from frame-slot load/store addressing.
- The new local tests are in `backend_aarch64_machine_printer`; no expectation,
  unsupported classification, runner, timeout, CTest registration, semantic
  handoff, or f128 transport changes were made.

## Proof

Ran:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)|backend_cli_dump_(bir|prepared_bir)_(00204_stdarg_(semantic|prepared)_handoff|focus_(function_filters|block_entry)_00204)|c_testsuite_aarch64_backend_src_002(04|16)_c)$'`

Result recorded in `test_after.log`: build succeeded; CTest ran 12 tests with
10 passing. `backend_aarch64_machine_printer`,
`backend_aarch64_target_instruction_records`, `backend_aarch64_instruction_dispatch`,
the prior 00204 handoff/dump guardrails, and `backend_lir_to_bir_notes` passed.
The remaining failures are:

- `c_testsuite_aarch64_backend_src_00216_c`: advanced past the assembler
  rejection and now fails with a runtime output mismatch. The actual output is
  mostly zeroed aggregate/global/local dumps and repeated `two` control-flow
  output where the expected output contains initialized aggregate contents and
  `one`, `two`, `three`.
- `c_testsuite_aarch64_backend_src_00204_c`: advanced to frame setup printing
  with `frame adjustment immediate is outside the plain #imm encoding range
  0..4095`.
