Status: Active
Source Idea Path: ideas/open/367_rv64_helper_free_variadic_entry_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define and Cover the Admission Contract / Implement the RV64 Admission Behavior

# Current Packet

## Just Finished

Completed Step 2/3, "Define and Cover the Admission Contract / Implement the
RV64 Admission Behavior."

RV64 object emission now admits a helper-free variadic entry only when its
prepared entry plan is fact-complete for the helper-free contract:

- `missing_required_facts=[]`
- `helper_resources.required_helpers=[]`
- `register_save_area.required=false`
- `overflow_area.required=true` with 8-byte alignment
- `va_list_layout.required=true`, size 8, align 8, and exactly one
  `overflow_arg_area` field at offset 0 size 8

`src/backend/mir/riscv/codegen/object_emission.cpp` preserves the existing
missing-plan and missing-required-facts diagnostics. Helper-bearing variadic
entries still flow past admission to the existing helper-specific diagnostics.
Incomplete helper-free facts now stop at:

```text
unsupported_function_admission: RV64 helper-free variadic entry requires a complete one-field overflow-area va_list contract
```

`tests/backend/mir/backend_riscv_object_emission_test.cpp` now covers:

- missing prepared variadic entry facts
- missing required target facts
- incomplete helper-free contract facts
- positive helper-free object emission without requiring helper-only
  overflow-area base slot or base stack offset
- existing `va_start` helper diagnostics and materialization

## Suggested Next

Execute Step 4 as the representative validation packet for `src/20030914-2.c`.
Run the delegated single-case RV64 gcc torture backend command and record the
next structured boundary in `todo.md`. Expected honest progress is that the
case no longer stops at helper-free variadic admission; the likely next stop is
the documented byval stack-slot parameter-home boundary from idea 363.

Suggested proof:

```sh
tmp=$(mktemp); printf 'src/20030914-2.c\n' > "$tmp"; ALLOWLIST="$tmp" CASE_TIMEOUT_SEC=20 STOP_ON_FAILURE=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log; rc=$?; rm -f "$tmp"; exit $rc
```

## Watchouts

- Do not reintroduce a requirement for `overflow_area.base_slot_id` or
  `overflow_area.base_stack_offset_bytes` on helper-free entries.
- Do not weaken helper-specific `va_start`, `va_arg`, `va_arg_aggregate`, or
  `va_copy` diagnostics; those remain separate boundaries.
- Do not route byval aggregate stack-slot parameter-home lowering into this
  helper-free admission idea.
- Representative validation may fail at a later structured boundary; that is
  acceptable for Step 4 if the old helper-free admission diagnostic is gone.

## Proof

Delegated proof passed:

```sh
cmake --build build --target backend_riscv_object_emission_test && ctest --test-dir build -R '^backend_riscv_object_emission$' --output-on-failure > test_after.log
```

`test_after.log` contains 1/1 passing test:
`backend_riscv_object_emission`.
