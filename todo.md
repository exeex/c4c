# Current Packet

Status: Active
Source Idea Path: ideas/open/149_residual_prepared_lookup_include_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Clean AArch64 residual includes

## Just Finished

Step 2 cleaned the removable AArch64 residual `prepared_lookups.hpp` includes
identified by the Step 1 map, replacing the facade with narrow prealloc owner
headers only:

- `src/backend/mir/aarch64/codegen/dispatch_lookup.cpp` now uses
  `control_flow.hpp` and `value_locations.hpp`.
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp` now uses
  `addressing.hpp` and its existing `select_chain_lookups.hpp`.
- `src/backend/mir/aarch64/codegen/frame_slot_address.cpp` now uses
  `addressing.hpp` plus its existing stack-layout header.
- `src/backend/mir/aarch64/codegen/fp_value_materialization.cpp` now uses
  `addressing.hpp`, `control_flow.hpp`, `names.hpp`, `value_locations.hpp`,
  and its existing `select_chain_lookups.hpp`.
- `src/backend/mir/aarch64/codegen/memory_store_retargeting.cpp` now uses
  `addressing.hpp`, `control_flow.hpp`, `value_locations.hpp`, and its
  existing stack-layout header.
- `src/backend/mir/aarch64/codegen/select_materialization.cpp` now uses
  `calls.hpp`, `control_flow.hpp`, and `value_locations.hpp`.
- `src/backend/mir/aarch64/codegen/comparison.cpp` now uses
  `addressing.hpp`, `control_flow.hpp`, `publication_plans.hpp`,
  `value_locations.hpp`, and its existing comparison/select-chain/stack-layout
  headers.
- `src/backend/mir/aarch64/codegen/calls.cpp` now uses `control_flow.hpp` and
  `publication_plans.hpp` while keeping its existing concrete prealloc headers.

No lowering logic, expectations, kept AArch64 files, or blocked
`dispatch_producers.cpp` include were changed. The remaining AArch64
`prepared_lookups.hpp` includes are only in the Step 1 keep/blocked set:
`alu.cpp`, `module/module.hpp`, and `dispatch_producers.cpp`.

## Suggested Next

Execute Step 3 as a prealloc include cleanup packet for the prealloc files
classified removable in the Step 1 map:
`call_plans.cpp`, `prepared_printer/select_chains.cpp`,
`select_chain_lookups.cpp`, `formal_publications.cpp`,
`decoded_home_storage.hpp`, and `module.hpp`.

## Watchouts

- Leave `src/backend/mir/aarch64/codegen/dispatch_producers.cpp` alone until
  the missing narrow declaration owner for
  `make_prepared_edge_publication_lookups` is resolved or explicitly accepted.
- Continue to keep `prepared_lookups.hpp` in files that name
  `PreparedFunctionLookups`, `make_prepared_function_lookups`, or the
  return-chain helpers that still have no narrow owner header.
- Keep Step 3 include-focused and behavior-preserving; do not move factory
  declarations or cleanup the blocked AArch64 case unless delegated.

## Proof

Proof command run:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: green. `test_after.log` records the build and backend test proof:
179/179 backend tests passed, 0 failed.

Additional include check:

`rg -n '#include .*prepared_lookups\.hpp' <owned AArch64 files plus keep/blocked files>`

Result: no matches in the eight owned removable AArch64 files; matches remain
only in `alu.cpp`, `module/module.hpp`, and `dispatch_producers.cpp` as
expected.
