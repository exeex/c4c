Status: Active
Source Idea Path: ideas/open/362_rv64_scalar_vararg_and_variadic_lowering.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Consume or Precisely Diagnose `va_copy`

# Current Packet

## Just Finished

Step 3 completed for `plan.md` Step 3.

Implemented RV64 `va_copy` prepared fact consumption in
`src/backend/prealloc/variadic_entry_plans.cpp`:

- RV64 now scans `VaCopy` at the same helper operand-home boundary as
  `VaStart`, scalar `VaArg`, and aggregate `VaArgAggregate`.
- When both `llvm.va_copy.p0.p0` operands have prepared value homes, RV64
  records the existing `destination_va_list` and `source_va_list` carrier and
  removes the old generic missing operand facts.
- Missing destination/source homes still publish precise
  `helper_operand_homes.va_copy.*` diagnostics through the shared
  `require_variadic_helper_operand_home` path.
- The focused RV64 tests now assert complete `VaCopy` operand homes while
  preserving the committed RV64 `VaStart`, scalar `VaArg`, and aggregate
  `VaArgAggregate` behavior.

## Suggested Next

Delegate Step 4 to narrow the RV64 object-emission variadic function admission
gate without changing the prepared producer contracts: inspect the broad
`RV64 object variadic function lowering is not implemented` path, consume the
now-complete prepared `VaStart`, scalar `VaArg`, aggregate `VaArgAggregate`,
and `VaCopy` facts where a minimal supported lowering path is explicit, or
replace the broad gate with precise unsupported diagnostics for the exact
remaining lowering class.

Suggested proof command:

```sh
cmake --build build --target backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer)$' --output-on-failure > test_after.log
```

## Watchouts

- The broad RV64 object-route admission gate remains intentionally untouched in
  this packet; Step 4 owns that lowering/admission work.
- Preserve the RV64 scalar integer `va_arg` overflow plan, aggregate overflow
  `va_arg` plan, and the new `VaCopy` operand-home carrier while changing
  object emission.
- `target_abi.va_arg.scalar_payload_abi` is now the intended precise diagnostic
  for operand-complete unsupported RV64 scalar payload classes such as the
  fixture's f64/SSE-shaped payload.
- Treat `src/20030914-2.c` and `src/920908-1.c` as representatives only; do
  not implement testcase-shaped shortcuts for Step 4.

## Proof

Proof command completed successfully; `test_after.log` is the proof log:

```sh
cmake --build build --target backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer)$' --output-on-failure > test_after.log
```

Result: both `backend_prepare_frame_stack_call_contract` and
`backend_prepared_printer` passed.

`git diff --check` also passed.
