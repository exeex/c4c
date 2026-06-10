# 149 Residual prepared lookup include cleanup

## Goal

Clean residual `prepared_lookups.hpp` include pressure after narrower owner
moves land.

## Why This Exists

Idea 141 found broad residual includes in AArch64 and prealloc files that only
need moved value-home, move-bundle, stack-layout, source-producer, publication,
or same-block materialization declarations. Include cleanup should happen after
the declarations have real narrower homes.

## In Scope

- Remove broad `prepared_lookups.hpp` includes from consumers that no longer
  need `PreparedFunctionLookups` or `make_prepared_function_lookups`.
- Update AArch64 consumers identified by the audit, including
  `dispatch_producers.cpp`, `dispatch_publication.cpp`,
  `dispatch_value_materialization.cpp`, `comparison.cpp`, `alu.cpp`,
  `memory.cpp`, `memory_store_retargeting.cpp`, `frame_slot_address.cpp`, and
  `fp_value_materialization.cpp` where relevant.
- Update prealloc consumers such as `formal_publications.*`,
  `decoded_home_storage.*`, `select_chain_lookups.cpp`,
  `publication_plans.cpp`, and `prepared_printer/select_chains.cpp` where
  relevant.

## Out Of Scope

- Performing include cleanup before declarations actually have narrower owner
  homes.
- Removing `prepared_lookups.hpp` where a consumer still names
  `PreparedFunctionLookups` or `make_prepared_function_lookups`.
- Changing behavior to satisfy include cleanup.

## Acceptance Criteria

- Consumers include the narrowest available owner headers after owner moves.
- No umbrella header hides the old prepared lookup facade under a different
  name.
- Proof includes `cmake --build --preset default` and
  `ctest --test-dir build -R '^backend_' --output-on-failure`.

## Closure Note

Closed after the residual include audit and cleanup completed. Remaining direct
`prepared_lookups.hpp` includes were classified in `todo.md` before closure.
The only AArch64 cleanup target still blocked by declaration ownership is
`src/backend/mir/aarch64/codegen/dispatch_producers.cpp`, which still calls
`make_prepared_edge_publication_lookups`; that helper remains declared only in
`prepared_lookups.hpp`. Follow-up idea
`ideas/open/150_edge_publication_lookup_declaration_owner.md` owns that
declaration-owner work.

## Reviewer Reject Signals

- Cleanup hides the old facade through a different broad umbrella header.
- Consumers lose required aggregate declarations.
- The patch changes lowering behavior while claiming to be include-only.
