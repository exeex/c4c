# Current Packet

Status: Active
Source Idea Path: ideas/open/81_aarch64_dispatch_edge_copy_owner_contraction.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Relocate narrow target-local emission helpers

## Just Finished

Completed `plan.md` Step 3 by moving only
`emit_prepared_va_list_field_carrier_to_register` from the AArch64
edge-copy implementation into the variadic owner.

Concrete changes:

- Removed the private helper definition from `dispatch_edge_copies.cpp`.
- Added the same helper declaration to `variadic.hpp`.
- Added the same helper definition to `variadic.cpp`, preserving signature and
  logic.
- Kept the existing edge load-local call path calling the helper through
  `variadic.hpp`.

## Suggested Next

Supervisor should choose the next Step 3 contraction packet or ask the plan
owner whether Step 3 is exhausted if no narrow target-local helper remains.

## Watchouts

- `variadic.cpp` now includes `prepared_value_home_materialization.hpp` because
  the relocated carrier helper emits from prepared value homes.
- `dispatch_edge_copies.cpp` still uses the helper from the existing
  `variadic.hpp` include; no edge call-site logic changed.

## Proof

Passed:

`bash -o pipefail -c "(cmake --build build --target c4c_backend backend_aarch64_instruction_dispatch_test c4cll -- -j2 && ctest --test-dir build -R 'backend_aarch64_instruction_dispatch|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy' --output-on-failure) 2>&1 | tee test_after.log"`

Proof log: `test_after.log`.
