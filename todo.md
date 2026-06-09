Status: Active
Source Idea Path: ideas/open/144_source_producer_same_block_materialization_owner.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Declaration Surface And Consumers

# Current Packet

## Just Finished

Completed Step 1, "Inspect Declaration Surface And Consumers", for the
same-block/source-producer ownership move.

Target declaration and owner map:

- Publication source-producer fact structs:
  `PreparedEdgePublicationSourceProducer` and
  `PreparedEdgePublicationSourceProducerLookups` are already declared in
  `src/backend/prealloc/publication_plans.hpp:205` and
  `src/backend/prealloc/publication_plans.hpp:217`. Chosen owner:
  `publication_plans.hpp`, because these are publication fact structs and are
  referenced by publication plans at
  `src/backend/prealloc/publication_plans.hpp:712`,
  `src/backend/prealloc/publication_plans.hpp:736`,
  `src/backend/prealloc/publication_plans.hpp:787`, and
  `src/backend/prealloc/publication_plans.hpp:812`.
- Publication source-producer lookup helpers:
  `make_prepared_edge_publication_source_producer_lookups` is still declared in
  `src/backend/prealloc/prepared_lookups.hpp:301`, while its definition already
  lives in `src/backend/prealloc/select_chain_lookups.cpp:216`.
  `find_indexed_prepared_edge_publication_source_producer` is still declared in
  `src/backend/prealloc/prepared_lookups.hpp:376`, while its definition already
  lives in `src/backend/prealloc/select_chain_lookups.cpp:317`. Chosen owner:
  `select_chain_lookups.hpp`.
- Current-block publication consumption query:
  `PreparedCurrentBlockPublicationConsumption` is declared in
  `src/backend/prealloc/prepared_lookups.hpp:132`.
  `find_prepared_current_block_publication_consumption` is declared in
  `src/backend/prealloc/prepared_lookups.hpp:381` and defined in
  `src/backend/prealloc/prepared_lookups.cpp:2257`. Chosen owner:
  `select_chain_lookups.hpp`.
- Same-block scalar query helpers:
  `PreparedSameBlockScalarProducer` is declared in
  `src/backend/prealloc/prepared_lookups.hpp:117`.
  `PreparedSameBlockValueMaterializationQuery` is declared in
  `src/backend/prealloc/prepared_lookups.hpp:124`.
  The three public `find_prepared_same_block_scalar_producer` overloads are
  declared in `src/backend/prealloc/prepared_lookups.hpp:390`,
  `src/backend/prealloc/prepared_lookups.hpp:400`, and
  `src/backend/prealloc/prepared_lookups.hpp:409`, with public definitions in
  `src/backend/prealloc/prepared_lookups.cpp:2303`,
  `src/backend/prealloc/prepared_lookups.cpp:2321`, and
  `src/backend/prealloc/prepared_lookups.cpp:2342`. Their shared internal
  implementation is `prepared_same_block_source_producer` at
  `src/backend/prealloc/prepared_lookups.cpp:852`. Chosen owner:
  `select_chain_lookups.hpp`.
- Same-block integer-constant query helpers:
  The public `evaluate_prepared_same_block_integer_constant` overloads are
  declared in `src/backend/prealloc/prepared_lookups.hpp:426` and
  `src/backend/prealloc/prepared_lookups.hpp:435`, with public definitions in
  `src/backend/prealloc/prepared_lookups.cpp:2426` and
  `src/backend/prealloc/prepared_lookups.cpp:2442`. Their recursive internal
  implementation is at `src/backend/prealloc/prepared_lookups.cpp:899`.
  Chosen owner: `select_chain_lookups.hpp`.

Direct consumers and include pressure:

- `src/backend/prealloc/select_chain_lookups.cpp` defines the producer lookup
  builders/finders but includes `prepared_lookups.hpp` for declarations today;
  this is the strongest declaration-move pressure toward
  `select_chain_lookups.hpp`.
- `src/backend/prealloc/publication_plans.cpp` calls
  `find_indexed_prepared_edge_publication_source_producer` at line 109 and
  `make_prepared_edge_publication_source_producer_lookups` at line 1337. It
  already includes `select_chain_lookups.hpp`, so it should not need the
  residual prepared facade for these helpers.
- `src/backend/prealloc/prepared_printer/select_chains.cpp` calls
  `find_indexed_prepared_edge_publication_source_producer` at line 97 and
  `make_prepared_edge_publication_source_producer_lookups` at line 178. It
  includes `prepared_lookups.hpp`, `publication_plans.hpp`, and
  `select_chain_lookups.hpp`; after the move, recheck whether
  `prepared_lookups.hpp` is still needed there.
- `src/backend/prealloc/call_plans.cpp` calls
  `make_prepared_edge_publication_source_producer_lookups` at line 2589 and
  already includes both `prepared_lookups.hpp` and `select_chain_lookups.hpp`;
  only remove the broad include if no residual prepared lookup APIs remain.
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp` consumes
  `PreparedSameBlockValueMaterializationQuery`,
  `PreparedSameBlockScalarProducer`,
  `find_prepared_same_block_scalar_producer`, and
  `evaluate_prepared_same_block_integer_constant` at lines 41, 50, 60, 73, and
  111. It already includes `select_chain_lookups.hpp`; it appears to be a good
  candidate to drop `prepared_lookups.hpp` after the move if no other prepared
  declarations are needed.
- `src/backend/mir/aarch64/codegen/fp_value_materialization.cpp` consumes
  `PreparedSameBlockScalarProducer`,
  `find_prepared_same_block_scalar_producer`, and
  `make_prepared_edge_publication_source_producer_lookups` at lines 48, 64, 74,
  and 77. It currently includes `prepared_lookups.hpp` only among the prealloc
  owners scanned; add the narrow owner in the code-moving packet.
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp` consumes same-block
  scalar and source-producer lookup helpers at lines 154, 171, 184, 187, 216,
  229, 233, and 284, but it also uses current-block join source facts and
  value-home lookups from `prepared_lookups.hpp`, so the broad include likely
  stays for now.
- `src/backend/mir/aarch64/codegen/calls.cpp` consumes
  `find_prepared_current_block_publication_consumption` at line 6628 and
  source-producer lookup helpers at lines 6505, 7819, and 8073. It currently
  includes `select_chain_lookups.hpp` but not `prepared_lookups.hpp`; the
  declaration move should make this dependency explicit in the narrow owner.
- `src/backend/mir/aarch64/codegen/alu.cpp` consumes same-block scalar helpers
  at lines 1658 and 1679 and producer lookups around line 2228, but still uses
  `PreparedFunctionLookups` and return-chain helpers, so
  `prepared_lookups.hpp` remains required.
- `src/backend/mir/aarch64/codegen/comparison.cpp`,
  `src/backend/mir/aarch64/codegen/memory.cpp`, and
  `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp` consume source
  producer lookup helpers, but the scan found other prepared/publication
  dependencies; do not use Step 2 or Step 3 for broad include cleanup there.

## Suggested Next

Execute Step 2 from `plan.md`: move the source-producer helper declarations,
current-block publication consumption query declarations, same-block scalar
query declarations, and same-block integer-constant query declarations to
`select_chain_lookups.hpp`; leave publication fact structs in
`publication_plans.hpp`.

## Watchouts

- This active idea is an ownership move only; do not change source-producer,
  publication, materialization, AArch64 emission, call, comparison, or memory
  behavior.
- Prefer `src/backend/prealloc/select_chain_lookups.hpp` for
  source-producer/same-block materialization declarations unless inspection
  proves a fact struct already belongs in `publication_plans.hpp`.
- Do not replace prepared facts with local AArch64 rediscovery scans.
- Do not mix in call-argument, comparison, load-local, or residual include
  cleanup work; those are separate open ideas.
- Keep `prepared_lookups.hpp` includes where a consumer still needs
  `PreparedFunctionLookups`, `make_prepared_function_lookups`, or another
  residual prepared lookup API.
- `select_chain_lookups.hpp` currently only forward-declares
  `PreparedEdgePublicationSourceProducer` and
  `PreparedEdgePublicationSourceProducerLookups`; moving same-block result
  structs by value will require including the publication fact definitions, not
  just forward declarations.
- `find_prepared_current_block_publication_consumption` and the same-block
  scalar/integer definitions still live in `prepared_lookups.cpp`; if Step 2
  keeps definitions stable, `prepared_lookups.cpp` will need to include the new
  public declaration owner.

## Proof

Inspection-only analysis slice. No build or tests were run, and no
`test_after.log` was required by the delegated proof contract.
