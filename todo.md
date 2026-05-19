Status: Active
Source Idea Path: ideas/open/321_aarch64_aggregate_va_arg_helper_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Aggregate Va Arg Helper Records

# Current Packet

## Just Finished

Step 1 localized the raw aggregate `va_arg` helper records without changing
implementation.

The generated external representative
`build/c_testsuite_aarch64_backend/src/00204.c.s` contains repeated un-commented
triples:

- `va.arg.aggregate source=overflow_arg_area va_list=... destination_payload=... payload_size=... payload_align=... copy_size=... copy_align=... scratch_registers=2 scratch_stack=0`
- `va.arg.aggregate.source field=overflow_arg_area field_offset=0 payload_offset=0 slot_size=... rsa_slot#... rsa_stack+... rsa_size=192 rsa_align=16 gp_base=0 fp_base=64 gp_slot=8 fp_slot=16`
- `va.arg.aggregate.progress field=overflow_arg_area field_offset=0 stride=... overflow_slot#... overflow_stack+... overflow_align=8`

The representative covers overflow-area aggregate payloads with sizes 4, 8, 12,
16, 24, 32, 48, and 64 bytes, including 4-, 8-, and 16-byte alignment cases.
The raw helper text is emitted directly by
`src/backend/mir/aarch64/codegen/variadic.cpp::print_variadic_call()` in the
`PreparedVariadicEntryHelperKind::VaArgAggregate` branch. That branch consumes
`VariadicAggregateVaArgRecord` fields built from the complete prepared
`aggregate_access_plan`, then returns three descriptive target lines instead of
executable AArch64 lowering.

Owning surfaces:

- Source/helper classification:
  `src/backend/mir/aarch64/codegen/variadic.cpp::variadic_entry_helper_kind()`
  maps `llvm.va_arg.aggregate` to
  `PreparedVariadicEntryHelperKind::VaArgAggregate`.
- Prepared data contract:
  `src/backend/prealloc/variadic.hpp::PreparedVariadicAggregateVaArgAccessPlan`
  supplies source class, destination payload home, source field/payload offset,
  source slot size, copy size/alignment, and progression field/stride.
- Machine record construction:
  `src/backend/mir/aarch64/codegen/variadic.cpp::complete_variadic_call_record()`
  populates `CallInstructionRecord::variadic_aggregate_va_arg` through
  `make_variadic_aggregate_va_arg_record()`.
- Machine opcode/mnemonic:
  `src/backend/mir/aarch64/codegen/calls.cpp::make_call_instruction()` selects
  `MachineOpcode::VariadicVaArgAggregate`, and
  `src/backend/mir/aarch64/codegen/instruction.cpp` maps that opcode to
  `MachinePrinterMnemonicKind::VariadicVaArgAggregate` /
  `va.arg.aggregate`.
- Current bad printer:
  `src/backend/mir/aarch64/codegen/variadic.cpp::print_variadic_call()` prints
  the three raw `va.arg.aggregate*` records.

Representative local tests already covering the pipeline are
`backend_aarch64_target_instruction_records`,
`backend_aarch64_instruction_dispatch`, and `backend_aarch64_machine_printer`;
the last currently asserts the descriptive aggregate helper records. The
external proof case is `c_testsuite_aarch64_backend_src_00204_c`.

## Suggested Next

Execute Step 2 by replacing the
`print_variadic_call()` aggregate branch with executable AArch64 source
selection, payload copy, and `va_list` progression. The smallest initial repair
surface is the `VaArgAggregate` branch in
`src/backend/mir/aarch64/codegen/variadic.cpp`; update focused
`backend_aarch64_machine_printer` expectations only to assert executable output
and absence of raw `va.arg.aggregate*` text, not to weaken coverage.

## Watchouts

- The prepared access plan is present and selected; this packet found no
  semantic admission, prepared-BIR, or instruction-dispatch blocker.
- Current generated records all use `source=overflow_arg_area`; do not bake in
  `00204.c`, `x21`, the observed stack slots, or one payload size. The plan and
  enum also name `register_save_area` and `explicit_payload` source classes.
- The repair must handle three responsibilities: load/select the source payload
  address, copy `copy_size` bytes to `destination_payload_home`, and advance
  the prepared progression field by `progression_stride_bytes`.
- Do not reopen F128 transport addressability; that owner is closed.
- Do not special-case `00204.c`, `stdarg`, `myprintf`, one HFA struct, one
  register, one stack slot, one helper index, or one emitted line.
- Do not weaken expectations, unsupported classifications, CTest registration,
  runner behavior, timeout policy, proof-log policy, or prior guardrails.
- Preserve the repairs from ideas 314, 315, 317, 318, 319, and 320.
- Keep global initializer emission separate unless it becomes the first bad
  fact after aggregate `va_arg` helper text is gone.

## Proof

Fresh localization proof was written to `test_after.log`:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)|backend_cli_dump_(bir|prepared_bir)_(00204_stdarg_(semantic|prepared)_handoff|focus_(function_filters|block_entry)_00204)|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: build succeeded; CTest returned failure because the current owner is
still unrepaired. The focused subset ran 11 tests, 10 passed, and the only
failure was `c_testsuite_aarch64_backend_src_00204_c`, where the assembler
rejects the raw `va.arg.aggregate`, `va.arg.aggregate.source`, and
`va.arg.aggregate.progress` lines as unexpected tokens.
