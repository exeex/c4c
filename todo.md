Status: Active
Source Idea Path: ideas/open/320_aarch64_f128_transport_addressability.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair F128 Transport Addressability

# Current Packet

## Just Finished

Step 2 repaired the localized F128 transport addressability gap in
`src/backend/mir/aarch64/codegen/f128.cpp`. `print_f128_transport()` now prints
16-byte F128 transport memory operands through a local printable-address helper:

- direct `[sp, #offset]` for encodable 16-byte frame slots
- reserved GP scratch address materialization for large frame-slot offsets
- reserved GP scratch symbol address materialization for F128 symbol-memory
  transports
- the same address helper for memory-backed carrier slots before moving through
  reserved scratch `q16`

Focused local coverage was added to
`tests/backend/mir/backend_aarch64_machine_printer_test.cpp` for large
memory-backed F128 frame-slot transports and symbol-memory F128 transports. The
representative no longer fails with
`f128 memory transport address is not printable`.

New first bad fact after the repair: `c_testsuite_aarch64_backend_src_00204_c`
now reaches generated assembly validation and fails because raw
`va.arg.aggregate`, `va.arg.aggregate.source`, and
`va.arg.aggregate.progress` note/helper text is emitted into
`build/c_testsuite_aarch64_backend/src/00204.c.s`; the assembler reports
`unexpected token in argument list` beginning at those lines.

## Suggested Next

Classify the new `va.arg.aggregate` raw helper-text assembler residual before
continuing idea 320. It appears outside F128 transport addressability and likely
belongs to the AArch64 aggregate `va_arg` helper printer/lowering surface, not
to F128 memory transport spelling.

## Watchouts

- Preserve the fixed HFA lane repair, caller-side variadic HFA lane handoff,
  and callee-side aggregate `va_arg` helper handoff from idea 319.
- Do not reopen scalar ALU immediate materialization, raw `va_start` helper
  text, frame adjustment materialization, stack-offset spelling, runner
  behavior, timeout behavior, proof-log policy, or c-testsuite expectations.
- Do not special-case `00204.c`, `stdarg`, `myprintf`, one HFA struct, one
  output line, one register, one stack slot, or one instruction index.
- The separate global data emission symptom still exists in generated assembly.
  Do not mix global initializer emission into this packet unless it becomes the
  first bad fact after `f128_transport` addressability advances.
- The AArch64 prepared dump also contains global-symbol F128 aggregate loads
  such as `base=global_symbol symbol=hfa34 offset=... size=16 align=16`.
  Do not fold global initializer emission into this repair, but keep symbol
  memory operands in mind if the first bad fact advances from frame-slot
  transport to symbol-address transport.
- Avoid a named `00204.c` or `%t262` fix. The repair should be a general
  `F128TransportRecord` addressability/materialization rule.
- The raw `va.arg.aggregate` residual is not a reason to reopen
  `f128_transport`; the focused proof advanced past the exact F128 printer
  diagnostic.

## Proof

Ran the exact delegated proof command and wrote output to `test_after.log`:

`cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R
'^(backend_lir_to_bir_notes|backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)|backend_cli_dump_(bir|prepared_bir)_(00204_stdarg_(semantic|prepared)_handoff|focus_(function_filters|block_entry)_00204)|c_testsuite_aarch64_backend_src_00204_c)$'`

Build succeeded. The focused CTest subset ran 11 tests: 10 passed, and the only
failure was `c_testsuite_aarch64_backend_src_00204_c`, now at raw
`va.arg.aggregate` assembly text. `backend_aarch64_machine_printer` passed with
the new F128 transport addressability coverage.
