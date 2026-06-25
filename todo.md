Status: Active
Source Idea Path: ideas/open/362_rv64_scalar_vararg_and_variadic_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Narrow the RV64 Variadic Function Admission Gate

# Current Packet

## Just Finished

Step 4 audit completed for `plan.md` Step 4.

Inspected the RV64 object variadic admission gate and representative logs:

- `src/backend/mir/riscv/codegen/object_emission.cpp` rejects every defined
  variadic BIR function at `prepared_function_to_object_function` before
  parameter-home, stack-frame, traversal, or instruction lowering runs.
- The diagnostic helper uses
  `prepare::find_prepared_variadic_entry_plan(prepared, function_name)`.
  Missing entry plans produce
  `missing variadic entry contract facts were not prepared`; plans with
  missing facts list `missing_required_facts=[...]`; fact-complete plans still
  get the broad suffix
  `RV64 object variadic function lowering is not implemented`.
- `src/backend/prealloc/variadic_entry_plans.cpp` already publishes RV64
  `va_list` layout, overflow-area state, helper resources, and complete helper
  operand homes for the currently supportable `VaStart`, scalar integer
  `VaArg`, aggregate overflow `VaArgAggregate`, and `VaCopy` cases.
- `src/backend/prealloc/variadic.hpp` and `src/backend/prealloc/module.hpp`
  provide the exact consumer surface for object emission:
  `PreparedVariadicEntryPlanFunction`,
  `PreparedVariadicEntryHelperOperandHomes`,
  `has_complete_prepared_variadic_*`, and
  `find_prepared_variadic_entry_helper_operand_homes`.
- `tests/backend/mir/backend_riscv_object_emission_test.cpp` has the focused
  RV64 object admission/emission fixture surface, but currently has no
  variadic-entry object admission test; the next packet should add one there.
- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp` and
  `tests/backend/bir/backend_prepared_printer_test.cpp` already prove the
  prepared fact side for RV64 helper families and should remain in the proof
  subset when object emission starts consuming those facts.
- Representative logs
  `build/rv64_gcc_c_torture_backend/src_20030914-2.c/case.log` and
  `build/rv64_gcc_c_torture_backend/src_920908-1.c/case.log` both currently
  fail with the broad fact-complete diagnostic:
  `unsupported_function_admission: variadic functions are not supported by the RV64 object route; RV64 object variadic function lowering is not implemented`.

## Suggested Next

Delegate Step 4 implementation to replace the broad admission gate with a
prepared-facts-aware variadic object consumer boundary in
`src/backend/mir/riscv/codegen/object_emission.cpp`.

Recommended smallest semantic step: do not globally admit fact-complete
variadic functions yet. Instead, narrow the broad gate by classifying the
first unsupported variadic lowering requirement from the prepared entry plan
and BIR helper call stream, then add a focused object-emission test for that
diagnostic. If the implementation finds a helper-free variadic function or a
function whose helper calls are all complete and trivially lowerable, admit
only that path; otherwise emit precise diagnostics such as unsupported
`va_start` materialization, scalar `va_arg` materialization, aggregate
`va_arg` materialization, or `va_copy` materialization rather than the current
function-wide message.

First implementation boundary:

- In `prepared_function_to_object_function`, replace the unconditional
  `if (function->is_variadic) return ...` with an admission helper that
  requires a prepared entry plan, preserves missing-fact diagnostics, and lets
  only fact-complete plans reach traversal.
- In `fragment_for_prepared_instruction`, before normal call lowering,
  recognize `llvm.va_start.p0`, `llvm.va_arg.*`, `llvm.va_copy.p0.p0`, and
  aggregate `va_arg` calls by consulting
  `find_prepared_variadic_entry_helper_operand_homes(*entry_plan,
  block_index, instruction_index)`.
- For this first packet, either emit only the single prepared helper class
  whose object fragment is explicit, or return a helper-class-specific
  `unsupported_variadic_helper_lowering: ...` diagnostic when no safe fragment
  is ready. Avoid any testcase-name, source-shape, or BIR-shape shortcut.

Suggested proof command:

```sh
cmake --build build --target backend_riscv_object_emission_test backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test c4cll && ctest --test-dir build -R '^(backend_riscv_object_emission|backend_prepare_frame_stack_call_contract|backend_prepared_printer)$' --output-on-failure > test_after.log && printf 'src/20030914-2.c\nsrc/920908-1.c\n' > /tmp/rv64-vararg-representatives.txt && CASE_TIMEOUT_SEC=20 ALLOWLIST=/tmp/rv64-vararg-representatives.txt scripts/check_progress_rv64_gcc_c_torture_backend.sh >> test_after.log 2>&1
```

## Watchouts

- `prepared_function_to_object_function` currently computes `has_call` after
  the variadic gate; admitting a variadic path means helper calls will trigger
  call-frame prologue decisions unless helper calls are excluded from that
  scan or lowered before generic call handling.
- `fragment_for_prepared_call` intentionally rejects
  `PreparedCallWrapperKind::DirectExternVariadic`; do not conflate variadic
  function entry/helper support with external variadic call support.
- The legacy textual RV64 codegen has `emit_va_start_impl`,
  `emit_va_arg_impl`, and `emit_va_copy_impl`, but prepared object emission
  should consume prepared homes/plans directly instead of re-inferring from
  source values.
- Keep `src/20030914-2.c` and `src/920908-1.c` as representatives only. The
  next focused test should be a synthetic RV64 object-emission fixture in
  `backend_riscv_object_emission_test.cpp`.

## Proof

Audit-only packet; no build was required because only `todo.md` changed.

```sh
sed -n '180,240p' src/backend/mir/riscv/codegen/object_emission.cpp
sed -n '2860,2920p' src/backend/mir/riscv/codegen/object_emission.cpp
sed -n '1000,1150p' src/backend/prealloc/variadic_entry_plans.cpp
sed -n '250,370p' src/backend/prealloc/variadic.hpp
sed -n '150,205p' src/backend/prealloc/module.hpp
rg -n 'rv64.*variadic|variadic.*rv64|target_abi\.va_arg|helper_operand_homes\.va_copy|helper_operand_homes\.va_arg' tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp tests/backend/bir/backend_prepared_printer_test.cpp
cat build/rv64_gcc_c_torture_backend/src_20030914-2.c/case.log
cat build/rv64_gcc_c_torture_backend/src_920908-1.c/case.log
```

Result: inspection found both representative logs still at the same
fact-complete broad admission diagnostic and identified the object-emission
helper consumer boundary above. Existing `test_after.log` was not overwritten
by this audit packet.
