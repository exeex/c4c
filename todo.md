Status: Active
Source Idea Path: ideas/open/150_edge_publication_lookup_declaration_owner.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Update The AArch64 Consumer

# Current Packet

## Just Finished

Completed plan Step 3 for idea 150. Updated
`src/backend/mir/aarch64/codegen/dispatch_producers.cpp` so the AArch64
dispatch producer includes the narrow declaration owner
`src/backend/prealloc/publication_plans.hpp` instead of the prepared-lookups
facade.

`dispatch_producers.cpp` no longer directly includes
`src/backend/prealloc/prepared_lookups.hpp`; it still includes
`select_chain_lookups.hpp` directly for the source-producer and select-chain
lookup helpers used in the file. The existing
`make_prepared_edge_publication_lookups` call sites now resolve through the
narrow owner declaration.

## Suggested Next

Execute plan Step 4, Audit Remaining Facade Includes, by checking remaining
direct `prepared_lookups.hpp` consumers for use sites that only need narrower
prealloc declaration owners. Keep the packet audit-focused and avoid moving
helper definitions or changing lowering behavior.

## Watchouts

`dispatch_producers.cpp` intentionally keeps `select_chain_lookups.hpp`; do
not treat that as a transitive dependency cleanup target. Keep
`prepared_lookups.cpp` owning the helper definitions unless a later lifecycle
packet explicitly changes implementation ownership.

## Proof

Ran supervisor-selected proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

Result: passed. The build completed and all 179 selected `backend_` tests
passed. Proof log: `test_after.log`.
