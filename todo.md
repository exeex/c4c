Status: Active
Source Idea Path: ideas/open/240_phase_e5_route3_memory_source_identity_adapter_follow_up.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Validate Target-Policy No-Change Surfaces

# Current Packet

## Just Finished

Step 4 validated target-policy no-change surfaces for the Route 3 same-block
global-load adapter.

Inspected files:

- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- `tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- `plan.md`
- `todo.md`

Validation evidence:

- The Route 3 adapter commit changes only
  `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp` and
  `todo.md`; no expectation files, baselines, helper names, supported-path
  statuses, unsupported contracts, wrapper outputs, or public prepared lookup
  APIs were changed.
- The implementation still obtains the policy-bearing prepared access through
  `prepare::find_prepared_same_block_global_load_access(...)` and emits through
  `emit_prepared_global_load_to_register(...)`.
- `route3_agreed_same_block_global_load_access(...)` only accepts a Route 3
  semantic identity when it agrees with the prepared access; it returns the
  Route 3 `load_global` pointer plus the existing prepared access record.
- No ownership moved into Route 3 for address formation, materialization,
  relocation, final operands, value homes, wrappers, diagnostics, fallback, or
  target emission policy.
- No whole `memory_accesses`, `PreparedFunctionLookups`, or
  `PreparedBirModule` retirement/deletion/privatization occurred.

## Suggested Next

Execute Step 5 by recording the Route 3 adapter result and deciding whether the
active idea is ready for plan-owner lifecycle review, without broadening into
additional Route 3 readers or aggregate retirement.

## Watchouts

- Do not delete, privatize, rename, or hide `memory_accesses`,
  `PreparedFunctionLookups`, or `PreparedBirModule`.
- Do not move address formation, materialization, relocation, final operands,
  value homes, wrappers, diagnostics, fallback, or target emission policy into
  Route 3 ownership.
- The adapter deliberately does not extend the existing `globals.cpp`
  `prepared_current_global_load_access(...)` helper because that helper
  compares policy-bearing fields outside this selected semantic reader.
- Step 4 is validation-only; it did not add implementation, test, expectation,
  or baseline changes.

## Proof

Supervisor-selected proof ran exactly and passed:

```bash
cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_prepared_memory_operand_records|backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$' --output-on-failure > test_after.log
```

Result: 3/3 tests passed in `test_after.log`
(`backend_aarch64_prepared_memory_operand_records`,
`backend_aarch64_instruction_dispatch`, and
`backend_prepared_lookup_helper`).
