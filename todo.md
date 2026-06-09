Status: Active
Source Idea Path: ideas/open/140_edge_copy_facade_split.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Adjust definitions and consumers to the split facade

# Current Packet

## Just Finished

Completed `plan.md` Step 3 implementation/include alignment for the split
edge-copy facade.

Moved target-neutral reusable edge-publication definitions from
`src/backend/prealloc/prepared_lookups.cpp` to
`src/backend/prealloc/publication_plans.cpp`: edge publication key/hash helpers,
block-entry parallel-copy publication matching helpers, aggregate stack source
authority, same-width i32 stack-source publication, indexed edge-publication
queries, edge-copy source facts, and their private validation/copy predicates.
The small scalar/aggregate width predicate used by aggregate stack source
authority moved with that helper.

Kept cached construction and aggregate wiring in `prepared_lookups.cpp`:
`make_prepared_edge_publication_lookups`, `make_prepared_function_lookups`,
`PreparedFunctionLookups::edge_publications`, and current-block join routing
definitions remain there.

Narrowed `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp` away from
`prepared_lookups.hpp` because it only needs the publication facade for this
slice. Made `src/backend/mir/aarch64/codegen/memory.hpp` explicitly include
`publication_plans.hpp` for the publication types in its public signatures.
Verified `dispatch_producers.cpp` and `memory.cpp` still need
`prepared_lookups.hpp` because they use prepared aggregate builders,
value-home lookups, or current-block routing helpers.

## Suggested Next

Execute Step 4 from `plan.md`: validate the facade split by searching for
leftover generic ownership leaks, duplicate wrapper surfaces, or consumers that
rescan predecessors/BIR/value locations instead of using prepared facts, then
run the required backend proof.

## Watchouts

- `publication_plans.cpp` now owns reusable edge/publication query definitions
  but still does not build cached `PreparedEdgePublicationLookups`; those maps
  remain prepared lookup wiring.
- Current-block join routing definitions still remain in `prepared_lookups.cpp`
  with the documented shared-header declarations, because they depend on
  `PreparedValueHomeLookups`, cached edge-publication lookup construction, and
  AArch64 current-block routing shape.
- `dispatch_producers.cpp` and `memory.cpp` retained broad prepared includes for
  real aggregate/builder dependencies; do not narrow those includes without
  moving or splitting the underlying prepared lookup APIs.
- No consumer was changed to rescan predecessors, BIR producers, or local value
  locations in place of prepared facts.

## Proof

`cmake --build --preset default` succeeded. No root-level proof log was created
because this packet explicitly said not to create test logs.
