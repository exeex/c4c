Status: Active
Source Idea Path: ideas/open/362_rv64_scalar_vararg_and_variadic_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Scalar Vararg and `va_copy` Boundary Facts

# Current Packet

## Just Finished

Step 1 audit completed for `plan.md` Step 1.

Boundary map:

- Scalar `va_arg` prepared producer: `src/backend/prealloc/variadic_entry_plans.cpp`
  currently records the required helper kind and missing fact names for
  `VaArg`, but `populate_rv64_variadic_entry_va_start_operand_home_authority`
  handles only `VaStart` and `VaArgAggregate`; the `VaArg` case falls through.
  Current focused consumers in
  `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp` and
  `tests/backend/bir/backend_prepared_printer_test.cpp` intentionally expect
  `helper_operand_homes.va_arg.source_va_list`,
  `helper_operand_homes.va_arg.scalar_result`, and
  `helper_operand_homes.va_arg.scalar_access_plan` to remain missing for RV64.
- `va_copy` prepared producer: the same prealloc file records required helper
  kind and missing fact names for `VaCopy`, but the RV64 population switch also
  falls through for `VaCopy`. The focused consumers intentionally expect
  `helper_operand_homes.va_copy.destination_va_list` and
  `helper_operand_homes.va_copy.source_va_list` to remain missing for RV64.
- Aggregate `va_arg` control: RV64 already materializes overflow
  `VaArgAggregate` operand homes and access plans through
  `make_rv64_aggregate_va_arg_access_plan`; the focused tests guard that the
  completed idea 361 aggregate facts do not regress while scalar/copy facts are
  still absent.
- RV64 admission consumer: `src/backend/mir/riscv/codegen/object_emission.cpp`
  rejects any variadic prepared function in
  `rv64_variadic_function_admission_diagnostic`. If the entry plan exists and
  has no missing facts, the diagnostic becomes
  `RV64 object variadic function lowering is not implemented`; otherwise it
  reports the exact missing fact list.
- Representative logs: latest `test_after.log` includes `src/20030914-2.c` and
  `src/920908-1.c`; both case logs under
  `build/rv64_gcc_c_torture_backend/` fail only at
  `unsupported_function_admission: variadic functions are not supported by the RV64 object route; RV64 object variadic function lowering is not implemented`.

## Suggested Next

Delegate Step 2 to implement the first narrow boundary in
`src/backend/prealloc/variadic_entry_plans.cpp`: make RV64 populate scalar
`VaArg` operand homes for the overflow-arg-area-only cases that match RV64's
single-field `va_list` layout, and either create an explicit scalar access plan
or retain a precise target ABI missing fact for non-overflow scalar classes.
Keep object-emission admission unchanged until the scalar facts reach the
prepared consumer surface.

Suggested proof command:

```sh
cmake --build build --target backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer)$' --output-on-failure > test_after.log
```

## Watchouts

- Do not start with the object-emission admission gate for Step 2: the focused
  RV64 scalar `va_arg` missing facts prove the prepared producer boundary is
  still incomplete before target lowering can consume scalar facts.
- Do not remove the aggregate `va_arg` RV64 overflow behavior or the idea 361
  facts while adding scalar handling.
- `va_copy` is a separate Step 3 packet; the audit found its producer boundary
  is also in `populate_rv64_variadic_entry_va_start_operand_home_authority`, but
  scalar `va_arg` is the first narrower implementation boundary.
- Treat `src/20030914-2.c` and `src/920908-1.c` as representatives only. Their
  current failure proves the broad admission gate, not a testcase-shaped scalar
  or copy fix.

## Proof

Audit-only packet; no build was required because only `todo.md` changed.

Inspection commands/results:

- `git status --short`: checked current dirty state before audit.
- `rg -n "20030914-2|920908-1|va_arg|va_copy|variadic function lowering|scalar" test_after.log build/rv64_gcc_c_torture_backend`: latest proof contains both representative cases, both failed.
- `sed -n '1,180p' build/rv64_gcc_c_torture_backend/src_20030914-2.c/case.log` and matching `src_920908-1.c/case.log`: both fail at the broad RV64 variadic function admission diagnostic.
- Focused source inspection covered
  `src/backend/prealloc/variadic_entry_plans.cpp`,
  `src/backend/mir/riscv/codegen/object_emission.cpp`,
  `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`, and
  `tests/backend/bir/backend_prepared_printer_test.cpp`.
