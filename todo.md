# Current Packet

Status: Complete
Source Idea Path: ideas/open/149_residual_prepared_lookup_include_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove cleanup and remaining facade uses

## Just Finished

Step 4 proved the residual include cleanup and classified the remaining
`prepared_lookups.hpp` includes.

Remaining direct includes are justified:

- `src/backend/mir/aarch64/module/module.hpp`: stores a
  `prepare::PreparedFunctionLookups*`.
- `src/backend/mir/aarch64/codegen/alu.cpp`: constructs
  `PreparedFunctionLookups` and calls return-chain helpers that still live in
  the facade.
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`: still calls
  `make_prepared_edge_publication_lookups`, which remains declared only by
  `prepared_lookups.hpp`.
- `src/backend/mir/riscv/codegen/emit.hpp`: directly accepts
  `PreparedFunctionLookups*`.
- `src/backend/mir/x86/x86.hpp`: stores `PreparedFunctionLookups` and calls
  `make_prepared_function_lookups`.
- `src/backend/prealloc/prepared_lookups.cpp`: owning implementation include.

No removable AArch64 or prealloc cleanup target still includes
`prepared_lookups.hpp`.

## Suggested Next

The active runbook steps are complete. Ask the plan owner to close idea 149 or
decide whether the remaining `dispatch_producers.cpp` declaration-owner
blocker should become a separate source idea.

## Watchouts

- `dispatch_producers.cpp` remains a real blocker to complete removal from all
  AArch64 cleanup targets because `make_prepared_edge_publication_lookups` has
  no narrow declaration owner yet.
- Return-chain helpers are also still facade-only and justify keeping
  `prepared_lookups.hpp` in `alu.cpp`.

## Proof

Residual include search:

`rg -n '#include .*prepared_lookups\.hpp' src/backend/mir/aarch64 src/backend/prealloc src/backend/mir/riscv src/backend/mir/x86`

Result: matches only in the six justified files listed above.

Facade-use confirmation:

`rg -n 'PreparedFunctionLookups|make_prepared_function_lookups|make_prepared_edge_publication_lookups|PreparedReturnChainLookups|find_prepared_return_chain' src/backend/mir/aarch64/module/module.hpp src/backend/mir/aarch64/codegen/alu.cpp src/backend/mir/aarch64/codegen/dispatch_producers.cpp src/backend/mir/riscv/codegen/emit.hpp src/backend/mir/x86/x86.hpp src/backend/prealloc/prepared_lookups.cpp`

Result: every remaining include maps to direct facade use, owning
implementation, or the recorded `dispatch_producers.cpp` blocker.

Build and backend proof:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: green. `test_after.log` records 179/179 backend tests passed, 0
failed.
