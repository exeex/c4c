Status: Active
Source Idea Path: ideas/open/139_addressing_lookup_ownership_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Adjust consumers to the narrowest stable ownership boundary

# Current Packet

## Just Finished

Step 2 moved the addressing/frame lookup declaration boundary to domain
headers without moving definitions or cached aggregate wiring.

- `src/backend/prealloc/addressing.hpp` now declares
  `PreparedAddressMaterializationLookups`, `PreparedMemoryAccessLookups`,
  `prepared_memory_access_position_key`, the two addressing lookup builders,
  indexed address-materialization and memory-access queries,
  `collect_prepared_address_materializations_for_block`,
  `PreparedSameBlockGlobalLoadAccess`, and the global-load access queries.
- `src/backend/prealloc/stack_layout/stack_layout.hpp` now declares
  `PreparedFrameAddressOffset` and the two indexed frame-address-offset
  queries in the parent `c4c::backend::prepare` namespace.
- `src/backend/prealloc/prepared_lookups.hpp` keeps
  `PreparedFunctionLookups`, `make_prepared_function_lookups`, and broad
  aggregate declarations, and includes the domain headers so existing broad
  consumers still compile.
- Definitions and `PreparedFunctionLookups` aggregate construction remain in
  `src/backend/prealloc/prepared_lookups.cpp`.

## Suggested Next

Execute Step 3 from `plan.md`: adjust direct consumers/includes to prefer the
new domain headers where they no longer need broad `prepared_lookups.hpp`,
without changing lookup definitions or prepared aggregate construction.

## Watchouts

- Do not change address lowering, frame layout, TLS/global relocation behavior,
  or store-source publication semantics.
- Do not replace prepared addressing facts with local scans or name matching.
- Keep `PreparedFunctionLookups` cached aggregate access intact.
- `PreparedSameBlockGlobalLoadAccess` moved to `addressing.hpp` because
  `std::optional<PreparedSameBlockGlobalLoadAccess>` requires a complete result
  type at the declaration site; `PreparedSameBlockScalarProducer` remains
  forward-declared there.
- `stack_layout/stack_layout.hpp` forward-declares
  `PreparedAddressMaterializationLookups` and `PreparedValueHomeLookups` to
  avoid pulling broad prepared lookup ownership into the frame-domain header.
- `prepared_lookups.hpp` still includes the new domain headers for
  compatibility, so Step 3 should remove broad includes only where a file's
  real dependencies are now narrower.

## Proof

`cmake --build --preset default` succeeded. No root-level proof log was
created because this packet explicitly listed root-level logs as Do Not Touch.
