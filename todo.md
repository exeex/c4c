Status: Active
Source Idea Path: ideas/open/139_addressing_lookup_ownership_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Adjust consumers to the narrowest stable ownership boundary

# Current Packet

## Just Finished

Step 3 adjusted/verified direct consumers against the new domain-header
boundary without changing lookup semantics.

- `src/backend/prealloc/stack_layout/coordinator.cpp` now directly includes
  `src/backend/prealloc/addressing.hpp` for the addressing facts it builds.
- `src/backend/mir/aarch64/codegen/globals.cpp` now directly includes
  `addressing.hpp` and `stack_layout/stack_layout.hpp`; its broad
  `prepared_lookups.hpp` include was removed because it only needs
  addressing/frame lookup queries through the new domain headers.
- `src/backend/mir/aarch64/codegen/memory.cpp` and
  `src/backend/mir/aarch64/codegen/alu.cpp` now directly include
  `addressing.hpp`, while retaining `prepared_lookups.hpp` because both still
  access broad cached aggregate fields such as memory accesses, edge
  publication/source-producer lookups, return chains, and/or
  `PreparedFunctionLookups` fallback construction.
- `src/backend/mir/aarch64/codegen/memory_store_retargeting.cpp` now directly
  includes `stack_layout/stack_layout.hpp`, while retaining
  `prepared_lookups.hpp` for `PreparedValueHomeLookups` and stack-object/frame
  helper APIs that still live in the broad lookup header.
- Verified `src/backend/prealloc/publication_plans.hpp` already directly
  includes `addressing.hpp` and still needs `prepared_lookups.hpp` for edge
  publication/source-producer lookup types and builders.
- Verified `src/backend/mir/aarch64/module/module.hpp`,
  `dispatch_value_materialization.cpp`, and `fp_value_materialization.cpp`
  still require broad prepared lookup declarations or aggregate source-producer
  APIs rather than only addressing/frame lookup facts.

## Suggested Next

Execute Step 4 from `plan.md`: run the final ownership-boundary audit and
acceptance proof for the addressing/frame lookup cleanup, checking that
remaining broad `prepared_lookups.hpp` includes are justified and that no
consumer rebuilt lookup maps or replaced prepared queries with local scans.

## Watchouts

- Do not change address lowering, frame layout, TLS/global relocation behavior,
  or store-source publication semantics.
- Do not replace prepared addressing facts with local scans or name matching.
- Keep `PreparedFunctionLookups` cached aggregate access intact.
- `memory_store_retargeting.cpp` cannot drop `prepared_lookups.hpp` yet because
  `PreparedValueHomeLookups`, `find_frame_slot_by_id`, and
  `find_stack_object_by_id` still live there.
- `memory.cpp`, `alu.cpp`, `dispatch_value_materialization.cpp`, and
  `fp_value_materialization.cpp` still use broad aggregate/source-producer
  lookup APIs; dropping `prepared_lookups.hpp` there would require a separate
  declaration-ownership change outside this packet.

## Proof

`cmake --build --preset default` succeeded. No root-level proof log was
created because this packet explicitly listed root-level logs as Do Not Touch.
