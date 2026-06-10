Status: Active
Source Idea Path: ideas/open/150_edge_publication_lookup_declaration_owner.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Move The Helper Declarations

# Current Packet

## Just Finished

Completed plan Step 2 for idea 150. Moved the two public
`make_prepared_edge_publication_lookups` declarations from
`src/backend/prealloc/prepared_lookups.hpp` to the narrow owner
`src/backend/prealloc/publication_plans.hpp`, next to
`PreparedEdgePublicationLookups`.

`prepared_lookups.hpp` no longer redeclares the helper and continues to expose
the declaration to facade users through its existing intentional
`publication_plans.hpp` include. `publication_plans.hpp` still does not include
`module.hpp`; the existing `PreparedBirModule` forward declaration was moved
earlier in the header so the reference overload can be declared beside the
edge-publication lookup type.

The internal pointer overload and both public forwarding definitions in
`src/backend/prealloc/prepared_lookups.cpp` were not moved or behavior-changed.

## Suggested Next

Execute plan Step 3 by updating the AArch64 dispatch producer include surface
so users that call `make_prepared_edge_publication_lookups` depend directly on
the narrow owner header instead of relying on `prepared_lookups.hpp`.

## Watchouts

`dispatch_producers.cpp` is reserved for Step 3 and may still need both
`publication_plans.hpp` for `PreparedEdgePublicationLookups` and
`select_chain_lookups.hpp` for select-chain helpers. Keep
`prepared_lookups.cpp` owning the helper definitions unless a later plan step
explicitly changes implementation ownership.

## Proof

Ran supervisor-selected proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

Result: passed. The build completed and all 179 selected `backend_` tests
passed. Proof log: `test_after.log`.
