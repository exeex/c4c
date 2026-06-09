Status: Active
Source Idea Path: ideas/open/140_edge_copy_facade_split.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Move or rename routing-oriented facade declarations

# Current Packet

## Just Finished

Completed `plan.md` Step 2 declaration-boundary split.

Reusable edge publication/source facts now have their primary declarations in
`src/backend/prealloc/publication_plans.hpp`: edge publication/source producer
statuses and facts, aggregate stack source authority, edge publication keys and
lookup tables, edge copy source facts, typed stack source publication records,
and shared edge/publication query declarations including
`prepare_same_width_i32_stack_source_publication`.

`src/backend/prealloc/prepared_lookups.hpp` now includes
`publication_plans.hpp` for those reusable types while keeping
`PreparedMoveBundleLookups`, cached edge publication lookup builders, and
`PreparedFunctionLookups` wiring declarations in the prepared lookup facade.
`src/backend/mir/aarch64/codegen/dispatch_producers.hpp` now depends on
`publication_plans.hpp` for `PreparedEdgePublicationSourceProducer`.

Current-block join routing declarations remain in `prepared_lookups.hpp` and
are documented there as AArch64 current-block routing conveniences because the
definitions still live in `prepared_lookups.cpp`, which this packet could not
touch.

## Suggested Next

Execute Step 3 from `plan.md`: move implementation ownership for reusable edge
publication/source helper definitions out of `prepared_lookups.cpp` only where
the dependency graph stays shared and target-neutral; keep cached construction
and `PreparedFunctionLookups` aggregate wiring in `prepared_lookups.*`.

## Watchouts

- `publication_plans.hpp` no longer includes `prepared_lookups.hpp`; maintain
  that direction if Step 3 moves definitions.
- `make_prepared_edge_publication_lookups`, `make_prepared_function_lookups`,
  and `PreparedFunctionLookups::edge_publications` remain cached prepared
  lookup wiring and should not be moved as part of reusable query helper
  cleanup.
- `prepare_same_width_i32_stack_source_publication` remains shared because
  RISC-V consumes it.
- Current-block join routing was not moved to `dispatch_producers.hpp` because
  its definitions remain in `prepared_lookups.cpp`; moving those declarations
  target-local should be paired with implementation ownership changes.

## Proof

`cmake --build --preset default` succeeded. No root-level proof log was
created because this packet explicitly listed root-level logs under
Do Not Touch.
