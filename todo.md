Status: Active
Source Idea Path: ideas/open/362_rv64_scalar_vararg_and_variadic_lowering.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Consume or Precisely Diagnose `va_copy`

# Current Packet

## Just Finished

Step 2 completed for `plan.md` Step 2.

Implemented RV64 scalar `va_arg` prepared fact consumption in
`src/backend/prealloc/variadic_entry_plans.cpp`:

- RV64 now records `VaArg` helper operand homes instead of falling through the
  helper switch.
- Integer scalar `va_arg` payloads with source/result homes and explicit
  payload ABI now get an overflow-arg-area scalar access plan against RV64's
  single-field `va_list` layout.
- Operand-complete scalar payloads outside that supported class now drop the
  old generic missing operand/access-plan facts and publish
  `target_abi.va_arg.scalar_payload_abi` as the precise remaining target ABI
  diagnostic.
- Completed RV64 `VaStart` and overflow `VaArgAggregate` behavior stayed
  covered by the focused tests.
- `VaCopy` remains intentionally unimplemented for Step 3; its
  `helper_operand_homes.va_copy.destination_va_list` and
  `helper_operand_homes.va_copy.source_va_list` missing facts are still
  explicit.

## Suggested Next

Delegate Step 3 to consume or precisely diagnose RV64 `va_copy` in
`src/backend/prealloc/variadic_entry_plans.cpp`: materialize source and
destination `va_list` homes for RV64's single-field layout where explicit, keep
unsupported cases as target ABI diagnostics, and leave object-emission
admission unchanged until copy facts reach the prepared consumer surface.

Suggested proof command:

```sh
cmake --build build --target backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer)$' --output-on-failure > test_after.log
```

## Watchouts

- Do not start with the object-emission admission gate for Step 3: the focused
  RV64 `va_copy` missing facts still prove the prepared producer boundary is
  incomplete before target lowering can consume copy facts.
- Preserve the new RV64 scalar integer `va_arg` overflow plan and the existing
  aggregate `va_arg` overflow behavior while adding copy handling.
- `target_abi.va_arg.scalar_payload_abi` is now the intended precise diagnostic
  for operand-complete unsupported RV64 scalar payload classes such as the
  fixture's f64/SSE-shaped payload.
- Treat `src/20030914-2.c` and `src/920908-1.c` as representatives only. Their
  current failure proves the broad admission gate, not a testcase-shaped scalar
  or copy fix.

## Proof

Proof command completed successfully; `test_after.log` is the proof log:

```sh
cmake --build build --target backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer)$' --output-on-failure > test_after.log
```

Result: both `backend_prepare_frame_stack_call_contract` and
`backend_prepared_printer` passed.
