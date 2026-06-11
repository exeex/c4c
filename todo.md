Status: Active
Source Idea Path: ideas/open/184_phase_e_route1_producer_constant_view_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate the selected consumer route-first

# Current Packet

## Just Finished

Step 3 migrated the selected recursive publication dependency check in
`value_publication_may_read_register_index(...)` to query
`route1_publication_source_producer_for_value(...)` first. Complete Route 1
same-block producer facts now drive the recursive cast/binary/select walk, while
the existing `prepared_same_block_publication_source_producer(...)` path remains
the fallback for unavailable, no-producer, or otherwise incomplete Route 1
answers.

The focused AArch64 dispatch fixture that clears prepared source-producer
indexes now directly checks that Route 1 exposes a recursive register dependency
for the scalar call argument and still returns false for an unrelated register.

## Suggested Next

Proceed to Step 4 equivalence proof for route-present, route-absent,
non-constant, no-producer, and cross-block behavior around the selected
consumer.

## Watchouts

The migrated consumer intentionally does not treat `NoProducer` as an
authoritative negative; any incomplete Route 1 view still falls through to the
prepared helper/home behavior. Target register, home, storage, move, and
machine-record policy remain in the existing AArch64/prepared paths.

## Proof

Step 3 ran and passed:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_prepared_lookup_helper)$'; } > test_after.log 2>&1`

Proof log: `test_after.log`.
