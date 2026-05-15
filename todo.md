Status: Active
Source Idea Path: ideas/open/232_aarch64_variadic_function_entry_carriers.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Expose Prepared Dumps And Focused Tests

# Current Packet

## Just Finished

Step 4, Expose Prepared Dumps And Focused Tests, completed the prepared dump
and focused-test slice for the AAPCS64 variadic function-entry carrier family.

Implementation notes:
- The prepared printer now emits a compact helper list on
  `helper_resources`, so `va_start`, generic `va_arg`, aggregate `va_arg`, and
  `va_copy` observations are visible in one dump line as well as in the
  existing per-helper detail lines.
- `backend_prepared_printer_test` now covers an AAPCS64 variadic entry fixture
  with named integer, FP, and aggregate parameters plus `llvm.va_start.p0`,
  `llvm.va_arg.i32`, `llvm.va_arg.f64`, `llvm.va_arg.aggregate`, and
  `llvm.va_copy.p0.p0` helper calls.
- `backend_prepare_liveness_test` keeps the helper calls observable through
  prepared liveness while asserting the entry carrier publishes the same
  named GP/FP counts, helper family, register-save, and `va_list` facts.
- `backend_prepare_frame_stack_call_contract_test` asserts those helper calls
  do not leak into call-boundary variadic metadata while the entry carrier
  retains the AAPCS64 save-area and overflow-area facts.

## Suggested Next

Start Step 5 by adding fail-closed AArch64 consumption guards for the prepared
variadic entry carrier, without reconstructing save-area, overflow-area,
`va_list`, or helper scratch facts in target-local lowering.

## Watchouts

- The current prepared carrier records generic `va_arg` and aggregate
  `va_arg` helper kinds, but does not retain a typed per-helper observation
  that distinguishes `llvm.va_arg.i32` from `llvm.va_arg.f64`; tests cover the
  generic helper fact and named GP/FP entry counts where current IR exposes
  them.
- The carrier still records required-but-missing prepared storage facts such as
  `register_save_area.slot_id`, `register_save_area.stack_offset_bytes`, and
  `overflow_area.base_stack_offset_bytes`; later packets should either allocate
  these in prepared/frame state or keep downstream AArch64 paths fail-closed.
- Helper scratch-resource counts remain unknown because this packet did not add
  a prepared scratch-policy authority.
- No AArch64 target lowering/printer files were touched; target consumption and
  diagnostics still belong to Step 5.

## Proof

Ran the delegated proof command:

`(cmake --build build --target backend_prepare_liveness_test backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test -j2 && ctest --test-dir build --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer)$') > test_after.log 2>&1`

Result: passed. `test_after.log` contains the successful build and three-test
CTest run.
