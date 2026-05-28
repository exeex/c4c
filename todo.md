Status: Active
Source Idea Path: ideas/open/61_aarch64_shared_same_block_materialization_authority.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Migrate Scalar And Load-Local Availability

# Current Packet

## Just Finished

Step 3 - Migrate Scalar And Load-Local Availability migrated
`dispatch_lookup.cpp` availability helpers off local AArch64 BIR scans.

Completed:

- `find_same_block_scalar_producer(...)` now consumes
  `prepare::find_prepared_same_block_scalar_producer(...)` and returns the
  prepared source instruction index only after the shared fact identifies a
  same-block binary producer; AArch64 keeps only the local opcode-support
  filter for call-argument scalar lowering.
- `has_same_block_load_local_producer(...)` now consumes
  `prepare::find_prepared_same_block_load_local_source_producer(...)` using
  prepared memory-access and edge-publication source lookup tables.
- Missing prepared context, missing prepared lookup tables, invalid names, or
  absent shared facts now fail closed rather than falling back to target-local
  reverse scans.

## Suggested Next

Step 4 - Migrate Recursive Value Materialization: update
`dispatch_value_materialization.cpp` to consume shared/prepared source facts
for recursive same-block materialization without reintroducing AArch64
semantic source discovery.

## Watchouts

`find_same_block_scalar_producer(...)` had no in-repo call sites at this step,
so the signature was narrowed from raw value name to `bir::Value` to allow the
prepared query to validate the value type. `has_same_block_load_local_producer`
requires `context.function.prepared_lookups`; if that table is absent, the
helper deliberately returns unavailable.

## Proof

`(cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Passed: 169/169 backend tests. Proof log: `test_after.log`.
