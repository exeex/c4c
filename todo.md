Status: Active
Source Idea Path: ideas/open/322_aarch64_va_start_destination_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Va Start Destination Publication

# Current Packet

## Just Finished

Step 1 localized the AArch64 `va_start` destination publication fault.

The bad record shape is a `VariadicVaStartRecord` whose
`destination_va_list` is copied directly from
`PreparedVariadicEntryHelperOperandHomes::destination_va_list` as a
`PreparedValueHomeKind::Register` home. In the representative generated
`myprintf`, that home prints as `x21`, but no instruction materializes `x21`
as the address of the local `va_list` object before the field stores:

- `build/c_testsuite_aarch64_backend/src/00204.c.s` lines near `myprintf`
  save `x21`, then emit `mov x0, x21`.
- The first `va_start` field stores are then `str x9, [x21]`,
  `str x9, [x21, #8]`, `str x9, [x21, #16]`, `str w9, [x21, #24]`, and
  `str w9, [x21, #28]`.
- Later aggregate `va_arg` lowering reads and progresses the same `x21`
  address, so the current first bad fact is the missing local `va_list`
  address publication before `va_start`, not aggregate `va_arg` helper text.

Owning code surfaces:

- `src/backend/prealloc/variadic_entry_plans.cpp`:
  `collect_variadic_entry_helper_operand_homes()` records the `llvm.va_start`
  first operand through `prepared_home_for_named_value()` as
  `homes.destination_va_list`.
- `src/backend/mir/aarch64/codegen/variadic.cpp`:
  `make_variadic_va_start_record()` requires and copies
  `homes.destination_va_list` into `VariadicVaStartRecord`.
- `src/backend/mir/aarch64/codegen/variadic.cpp`:
  `print_va_start_lowering_lines()` currently accepts only a register home and
  treats `destination_va_list.register_name` as the base address for all
  `va_list` field stores.
- `src/backend/mir/aarch64/codegen/calls.cpp`:
  `make_call_instruction()` records the destination as a value def and memory
  write, but no frame-slot address publication is visible before the printer
  stores through the register.

Representative proof tests for the repair are the focused `00204.c` handoff
and AArch64 printer/dispatch subset:

- `backend_cli_dump_bir_00204_stdarg_semantic_handoff`
- `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff`
- `backend_cli_dump_bir_focus_function_filters_00204`
- `backend_cli_dump_prepared_bir_focus_function_filters_00204`
- `backend_cli_dump_bir_focus_block_entry_00204`
- `backend_cli_dump_prepared_bir_focus_block_entry_00204`
- `backend_aarch64_machine_printer`
- `backend_aarch64_instruction_dispatch`
- `backend_aarch64_target_instruction_records`
- `backend_lir_to_bir_notes`
- `c_testsuite_aarch64_backend_src_00204_c`

## Suggested Next

Step 2 should materialize or publish the writable local `va_list` destination
address before `print_va_start_lowering_lines()` emits field stores. The repair
should make the record/printer path use an actual local `va_list` address
instead of assuming the register value home already contains one.

## Watchouts

- Do not implement the repair by special-casing `x21`, `myprintf`,
  `00204.c`, or one observed frame offset. The missing capability is general
  AArch64 `va_start` destination address materialization for register-backed
  destination homes.
- `rg 'va\\.arg\\.aggregate' build/c_testsuite_aarch64_backend/src/00204.c.s`
  returns no matches after this packet.
- The external representative now reaches runtime, so the prior raw helper
  text assembler blocker is gone.
- Do not reopen F128 transport addressability; that owner is closed.
- Do not reopen aggregate `va_arg` helper lowering; that owner is closed.
- Do not special-case `00204.c`, `stdarg`, `myprintf`, `x21`, one local
  variable, one frame slot, one offset, or one emitted instruction sequence.
- Do not weaken expectations, unsupported classifications, CTest registration,
  runner behavior, timeout policy, proof-log policy, or prior guardrails.
- Preserve the repairs from ideas 314, 315, 317, 318, 319, 320, and 321.
- Keep global initializer emission and later runtime mismatches separate unless
  they become the first bad fact after `va_start` destination publication is
  repaired.

## Proof

Reran the delegated focused proof to `test_after.log`:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)|backend_cli_dump_(bir|prepared_bir)_(00204_stdarg_(semantic|prepared)_handoff|focus_(function_filters|block_entry)_00204)|c_testsuite_aarch64_backend_src_00204_c)$'
```

Result: build succeeded. CTest ran 11 tests: 10 passed and 1 failed. The only
failure is `c_testsuite_aarch64_backend_src_00204_c`, still
`[RUNTIME_NONZERO] exit=Segmentation fault`. This is the smallest focused proof
scope for the Step 2 repair because it preserves the semantic/prepared handoff,
printer, dispatch, and external representative surfaces around the
`va_start` destination publication fault.
