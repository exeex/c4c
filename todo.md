Status: Active
Source Idea Path: ideas/open/367_rv64_helper_free_variadic_entry_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Helper-Free Variadic Entry Facts

# Current Packet

## Just Finished

Completed Step 1, "Audit Helper-Free Variadic Entry Facts."

The exact RV64 admission gate is
`rv64_variadic_function_admission_diagnostic()` in
`src/backend/mir/riscv/codegen/object_emission.cpp`. It rejects variadic
functions before normal prepared-function lowering when:

- `find_prepared_variadic_entry_plan()` returns no entry plan.
- `missing_required_facts` is non-empty.
- `helper_resources.required_helpers` is empty.

The current `src/20030914-2.c` representative stops at the third branch:

```text
unsupported_function_admission: RV64 helper-free variadic entry lowering remains unsupported without an explicit supported variadic entry runtime contract
```

AST-backed lookup confirmed that the gate is called only from
`prepared_function_to_object_function()` and consumes only the prepared
variadic entry plan. The producer path in
`populate_variadic_entry_plans()` publishes RV64 target facts through
`populate_rv64_variadic_entry_abi_facts()`, clears the generic non-AAPCS64
missing target ABI facts, and intentionally keeps
`attach_rv64_variadic_entry_storage_authority()` helper-only. For a helper-free
entry, the prepared facts are therefore:

- `missing_required_facts=[]`
- `helper_resources.required_helpers=[]`
- `register_save_area.required=false`
- `overflow_area.required=true`, `align=8`, with no required base slot/state
- `va_list_layout.required=true`, `size=8`, `align=8`, one
  `overflow_arg_area` field at offset 0 size 8

That matches the focused producer/printer contract in
`backend_prepared_printer_test.cpp` and the object-emission fixture
`make_prepared_variadic_helper_free_complete_module()`, except the object
emission fixture manually includes an overflow base slot/offset even though
the helper-free producer does not require one. Prior RV64 variadic helper
contracts are stricter only when helpers are present: `va_start` needs
overflow-area initial base state and destination `va_list` address homes;
`va_arg`, `va_arg_aggregate`, and `va_copy` remain separate helper boundaries.
The helper-free case has no helper calls to materialize and should be admitted
once the target ABI entry/layout facts are complete.

## Suggested Next

Execute Step 2/3 as one shippable admission-contract packet: make RV64 admit a
helper-free variadic entry when the prepared entry plan exists, has no missing
facts, has no required helpers, has the RV64 one-field overflow-arg-area
`va_list` layout, and has no register-save-area requirement. Preserve the
existing missing-plan and missing-required-facts diagnostics, and keep helper
entries flowing to the existing helper diagnostic path.

Executor packet:

- Owned files: `src/backend/mir/riscv/codegen/object_emission.cpp`,
  `tests/backend/mir/backend_riscv_object_emission_test.cpp`, and `todo.md`.
- Do not edit BIR producers, prepared-printer tests, allowlists, `plan.md`, or
  idea files unless a focused test proves the producer contract is actually
  missing.
- Replace `rejects_fact_complete_helper_free_variadic_entry_without_runtime_contract()`
  with positive object-emission coverage for the fact-complete helper-free
  fixture; a minimal return-zero text body is sufficient.
- Add one negative fixture or assertion that preserves structured rejection for
  incomplete helper-free facts, if the existing missing-plan and missing-fact
  tests do not already cover the changed branch.
- Validate with:

```sh
cmake --build build --target backend_riscv_object_emission_test && ctest --test-dir build -R '^backend_riscv_object_emission$' --output-on-failure > test_after.log
```

## Watchouts

- Do not weaken the explicit variadic runtime-contract gate.
- Do not route `va_arg`, `va_copy`, or byval stack-slot parameter-home work into
  this idea unless the audit proves it is required by the helper-free entry
  admission contract.
- Do not use testcase-name matching for `src/20030914-2.c`.
- Do not require `overflow_area.base_slot_id` or
  `overflow_area.base_stack_offset_bytes` for helper-free admission; current
  RV64 producers publish those only for helper entries.
- Do not claim representative progress until Step 4 reruns `src/20030914-2.c`;
  this audit predicts the next stop should move past helper-free admission and
  likely reach the already documented byval stack-slot parameter-home boundary.

## Proof

Audit validation:

- Used `c4c-clang-tool-ccdb` for `rv64_variadic_function_admission_diagnostic()`,
  `populate_variadic_entry_plans()`, and
  `make_prepared_variadic_helper_free_complete_module()`.
- Representative log inspected:
  `build/rv64_gcc_c_torture_backend/src_20030914-2.c/case.log`.
- `git diff --check -- todo.md` passed.
