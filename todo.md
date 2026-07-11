Status: Active
Source Idea Path: ideas/open/699_prealloc_named_bir_proof_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Confirm Prepared Authority Boundaries

# Current Packet

## Just Finished

Repaired the `plan.md` Step 2 recorded future proof command while preserving
the completed `plan.md` Step 2 authority-boundary confirmation for
`prepare::attribute_route4_block_entry_publication_if_agreeing`.

The data produced inside that helper is diagnostic/agreement-only:
`route4_block_entry_publication_status`,
`route4_block_entry_publication_route_status`,
`route4_block_entry_publication_instruction_index`, and
`route4_block_entry_publication_attributed`. Those fields are populated only
after the prepared lookup has already found an available
`PreparedCurrentBlockEntryPublication`, an available
`PreparedBlockEntryPublication`, a destination home/name/id, and a prepared
move bundle. Route 4 success therefore confirms BIR agreement with the already
selected prepared publication; it does not authorize execution.

The executable authority remains the prepared records:
`PreparedValueLocationFunction::move_bundles`, the selected
`PreparedMoveBundle`, the selected `PreparedMoveResolution`, the selected
`PreparedValueHome`, the destination value id/name/home, destination storage
kind/register name, and the `PreparedBlockEntryPublicationStatus::Available`
predicate produced by `collect_prepared_block_entry_publications`. Adjacent
edge-publication authority remains in `PreparedEdgePublication` records and
their source producer, source home, source memory, parallel-copy, move-bundle,
aggregate stack-source, and freshness fields; Route 4 status must not replace
those records.

The previously recorded CTest filter,
`ctest --test-dir build -R '^backend_prealloc_block_entry_publications$'
--output-on-failure`, is not runnable in this checkout because the active
CTest registry returns zero tests for that exact name. The focused test binary
`./build/tests/backend/bir/backend_prealloc_block_entry_publications_test`
exists and runs successfully, so it is the narrowest currently runnable proof
surface for the selected prepared current block-entry publication lookup path.

## Suggested Next

Add or reuse the block-entry
`bir::validate_block_entry_publication_reference(const BirPublicationView&,
const Block&, const Value&)` adapter, then migrate only
`attribute_route4_block_entry_publication_if_agreeing` to call the named view
while preserving the existing attribution fields and fail-closed behavior.

## Watchouts

`tests/backend/bir/CMakeLists.txt` contains an `add_test` entry for
`backend_prealloc_block_entry_publications`, but the current `build/` CTest
inventory does not register it. Until the build-tree registration mismatch is
fixed or regenerated in a way that exposes the CTest, use the direct binary
command below for the focused future proof.

Do not copy Route 4 validity, route status, instruction index, successor
identity, PHI identity, or destination type into prepared authority. The
future implementation should keep prepared consumer gates on
`PreparedCurrentBlockEntryPublicationStatus::Available`,
`prepared_block_entry_publication_available`, non-null destination home,
prepared move bundle/move, and destination register availability.

Preserve fail-closed outcomes for missing BIR successor/value evidence, wrong
destination type, wrong successor, duplicate Route 4 records, stale owner
records, diverged BIR instruction pointers, and BIR-only PHI identity that
lacks prepared move publication readiness. The prepared-printer path calls
this same prepared lookup through
`find_agreeing_route4_block_entry_publication`, so leave printer vocabulary and
dump rows unchanged until a later packet explicitly migrates that consumer.

## Proof

No build or CTest proof was required for this proof-command repair packet, per
delegated proof.

Checked runnable proof surfaces:
`ctest --test-dir build -N -R '^backend_prealloc_block_entry_publications$'`
reported `Total Tests: 0`, while
`./build/tests/backend/bir/backend_prealloc_block_entry_publications_test`
exited 0.

Future implementation proof command:
`cmake --build --preset default && ./build/tests/backend/bir/backend_prealloc_block_entry_publications_test`.

No `test_after.log` was produced because this packet only repairs the recorded
future proof command in `todo.md`.
