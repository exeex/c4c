Status: Active
Source Idea Path: ideas/open/320_aarch64_f128_transport_addressability.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate And Classify Residuals

# Current Packet

## Just Finished

Step 4 classified the post-Step-2 residual. The remaining
`c_testsuite_aarch64_backend_src_00204_c` failure is outside idea 320's
`f128_transport` addressability owner.

Concrete evidence:

- The focused proof no longer reports `family=f128_transport`,
  `opcode=f128_transport`, or `f128 memory transport address is not printable`.
- `test_before.log` and the refreshed `test_after.log` both show the only
  failing test is `c_testsuite_aarch64_backend_src_00204_c`, now at raw
  `va.arg.aggregate` assembler text.
- The generated assembly at
  `build/c_testsuite_aarch64_backend/src/00204.c.s` contains un-commented lines
  such as `va.arg.aggregate source=overflow_arg_area ...`,
  `va.arg.aggregate.source ...`, and `va.arg.aggregate.progress ...` among
  otherwise ordinary AArch64 instructions.
- The assembler rejects those lines with `unexpected token in argument list`.
- The printing source is the selected aggregate variadic helper path:
  `src/backend/mir/aarch64/codegen/variadic.cpp::print_variadic_call()` for
  `PreparedVariadicEntryHelperKind::VaArgAggregate`. That function currently
  returns descriptive record text for `va.arg.aggregate` instead of executable
  AArch64 lowering. The mnemonic comes from
  `MachinePrinterMnemonicKind::VariadicVaArgAggregate`, not from
  `F128TransportRecord` or `print_f128_transport()`.

Classification: idea 320 is complete for the current owner. The raw
`va.arg.aggregate` residual should be split into a new lifecycle owner for
AArch64 aggregate `va_arg` helper lowering/printing, not folded into F128
transport addressability.

## Suggested Next

Ask the plan owner to close idea 320 and create/activate a follow-up idea for
AArch64 aggregate `va_arg` helper lowering so `VariadicVaArgAggregate` machine
nodes emit executable copy/progression code instead of raw record text.

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
- Do not claim idea 320 owns aggregate `va_arg` runtime semantics. Its completed
  owner is F128 memory transport addressability; aggregate `va_arg` lowering is
  a separate variadic helper capability.

## Proof

Reran the exact delegated proof command and wrote output to `test_after.log`:

`cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R
'^(backend_lir_to_bir_notes|backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)|backend_cli_dump_(bir|prepared_bir)_(00204_stdarg_(semantic|prepared)_handoff|focus_(function_filters|block_entry)_00204)|c_testsuite_aarch64_backend_src_00204_c)$'`

Build succeeded. The focused CTest subset ran 11 tests: 10 passed, and the only
failure was `c_testsuite_aarch64_backend_src_00204_c`, at raw
`va.arg.aggregate` assembly text. The F128 transport addressability failure is
gone.
