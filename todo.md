# Current Packet

Status: Complete
Source Idea Path: ideas/open/149_residual_prepared_lookup_include_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Clean prealloc residual includes

## Just Finished

Step 3 finished cleaning the removable prealloc residual
`prepared_lookups.hpp` includes identified by the Step 1 map.

- `src/backend/prealloc/call_plans.cpp` now relies on its existing concrete
  owner includes and no longer includes `prepared_lookups.hpp`.
- `src/backend/prealloc/prepared_printer/select_chains.cpp` now relies on its
  existing `private.hpp`, `publication_plans.hpp`, and
  `select_chain_lookups.hpp` includes.
- `src/backend/prealloc/select_chain_lookups.cpp` now relies on
  `select_chain_lookups.hpp` and `module.hpp`.
- `src/backend/prealloc/formal_publications.cpp` now uses the narrow owners
  `control_flow.hpp` for `resolve_prepared_value_name_id` and
  `value_locations.hpp` for value-home lookup declarations.
- `src/backend/prealloc/decoded_home_storage.hpp` now relies on its existing
  `regalloc.hpp`, `storage.hpp`, and `value_locations.hpp` includes.
- `src/backend/mir/riscv/codegen/emit.hpp` and
  `src/backend/mir/x86/x86.hpp` now include `prepared_lookups.hpp` directly
  because they name `PreparedFunctionLookups` and/or
  `make_prepared_function_lookups`.
- `src/backend/prealloc/module.hpp` no longer includes
  `prepared_lookups.hpp`; it only keeps the narrow owners needed by
  `PreparedBirModule` and inline helpers.

## Suggested Next

Execute Step 4 validation: search the AArch64/prealloc target sets for residual
`prepared_lookups.hpp` includes, confirm remaining matches are keep/blocked or
true direct facade users, and rerun the supervisor-selected backend proof.

## Watchouts

- The initial Step 1 map for `formal_publications.cpp` missed
  `control_flow.hpp`; the compile failure showed that
  `resolve_prepared_value_name_id` is also used there.
- Leave `src/backend/mir/aarch64/codegen/dispatch_producers.cpp` alone until
  the missing narrow declaration owner for
  `make_prepared_edge_publication_lookups` is resolved or explicitly accepted.
- Continue to keep `prepared_lookups.hpp` in files that name
  `PreparedFunctionLookups`, `make_prepared_function_lookups`, or the
  return-chain helpers that still have no narrow owner header.

## Proof

Proof command run:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: green. `test_after.log` records the fresh delegated proof: build
succeeded and 179/179 backend tests passed, 0 failed.

Additional include check:

`rg -n '#include .*prepared_lookups\.hpp|PreparedFunctionLookups|make_prepared_function_lookups' src/backend/prealloc/module.hpp src/backend/mir/riscv/codegen/emit.hpp src/backend/mir/x86/x86.hpp`

Result: `module.hpp` has no match. The direct matches are in
`emit.hpp` and `x86.hpp`, which now include `prepared_lookups.hpp`
themselves.
