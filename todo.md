Status: Active
Source Idea Path: ideas/open/362_rv64_scalar_vararg_and_variadic_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Narrow the RV64 Variadic Function Admission Gate

# Current Packet

## Just Finished

Step 4 implementation completed for `plan.md` Step 4.

Replaced the broad fact-complete variadic admission diagnostic in
`src/backend/mir/riscv/codegen/object_emission.cpp` with a prepared-facts-aware
boundary:

- Missing variadic entry plans still reject with
  `missing variadic entry contract facts were not prepared`.
- Entry plans with missing required facts still reject with
  `missing_required_facts=[...]`.
- Fact-complete variadic definitions now pass the old broad admission gate and
  inspect the BIR helper call stream.
- Recognized prepared variadic helpers consult
  `find_prepared_variadic_entry_helper_operand_homes` and
  `has_complete_prepared_variadic_entry_helper_operand_homes` before reporting
  a helper-class-specific diagnostic such as
  `unsupported_variadic_helper_lowering: RV64 object route does not yet lower va_start helper`.
- The helper scan runs before ordinary parameter-home rejection for variadic
  definitions, so fact-complete helper users reach the helper boundary first.

Added focused RV64 object-emission tests in
`tests/backend/mir/backend_riscv_object_emission_test.cpp` for:

- preserving the missing-entry-plan diagnostic,
- preserving the missing-required-facts diagnostic,
- rejecting a fact-complete `llvm.va_start.p0` helper with the new
  helper-specific diagnostic.

## Suggested Next

Delegate the next packet to materialize the first explicit RV64 object helper
lowering fragment, starting with `va_start` if the prepared `va_list` and
overflow-area homes are sufficient. Keep direct external variadic call
rejection unchanged and add one focused object-emission test that proves the
helper fragment, not just a diagnostic rename.

## Watchouts

- `src/920908-1.c` now fails at the new helper boundary:
  `unsupported_variadic_helper_lowering: RV64 object route does not yet lower va_start helper`.
- `src/20030914-2.c` now moves past the broad variadic gate to the existing
  `unsupported_param_home: RV64 object route requires all parameters in supported GPR homes`
  boundary; that appears to be a separate parameter-home support issue, not a
  helper-call lowering case.
- Admitting a real helper lowering path will need to revisit whether helper
  calls should count as ordinary calls for call-frame prologue decisions.
- `fragment_for_prepared_call` intentionally rejects
  `PreparedCallWrapperKind::DirectExternVariadic`; do not conflate variadic
  function entry/helper support with external variadic call support.
- The legacy textual RV64 codegen has `emit_va_start_impl`,
  `emit_va_arg_impl`, and `emit_va_copy_impl`, but prepared object emission
  should consume prepared homes/plans directly instead of re-inferring from
  source values.
- Keep `src/20030914-2.c` and `src/920908-1.c` as representatives only.

## Proof

Focused proof passed, and the representative scan was preserved in
`test_after.log`.

```sh
cmake --build build --target backend_riscv_object_emission_test backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test c4cll && ctest --test-dir build -R '^(backend_riscv_object_emission|backend_prepare_frame_stack_call_contract|backend_prepared_printer)$' --output-on-failure > test_after.log && printf 'src/20030914-2.c\nsrc/920908-1.c\n' > /tmp/rv64-vararg-representatives.txt && CASE_TIMEOUT_SEC=20 ALLOWLIST=/tmp/rv64-vararg-representatives.txt scripts/check_progress_rv64_gcc_c_torture_backend.sh >> test_after.log 2>&1
```

Result: build and focused CTests passed; representative scan exited nonzero
because both representative cases still fail.

Allowed rerun with `|| true` only around the representative scan:

```sh
cmake --build build --target backend_riscv_object_emission_test backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test c4cll && ctest --test-dir build -R '^(backend_riscv_object_emission|backend_prepare_frame_stack_call_contract|backend_prepared_printer)$' --output-on-failure > test_after.log && printf 'src/20030914-2.c\nsrc/920908-1.c\n' > /tmp/rv64-vararg-representatives.txt && (CASE_TIMEOUT_SEC=20 ALLOWLIST=/tmp/rv64-vararg-representatives.txt scripts/check_progress_rv64_gcc_c_torture_backend.sh >> test_after.log 2>&1 || true)
```

Result: command completed; `test_after.log` records 3/3 focused CTests passed
and representative totals `total=2 passed=0 failed=2`.
