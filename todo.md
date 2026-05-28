Status: Active
Source Idea Path: ideas/open/61_aarch64_shared_same_block_materialization_authority.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Define Shared Same-Block Source Facts

# Current Packet

## Just Finished

Step 2 - Define Shared Same-Block Source Facts added public shared/prepared
facts without migrating AArch64 consumers.

Added:

- `prepare::find_prepared_same_block_scalar_producer(...)` overload that takes a
  `bir::Value`, resolves the prepared value name internally, and reuses the
  existing prepared source-producer validation.
- Public `prepare::evaluate_prepared_same_block_integer_constant(...)` query
  backed by the existing prepared recursive evaluator. Missing prepared names,
  missing source-producer facts, producer/index/pointer mismatches,
  non-binary producers, divide-by-zero, invalid shifts, and recursion-limit
  cases continue to return `std::nullopt`.
- Focused shared-level coverage in `backend_prepared_lookup_helper_test` for
  value-based scalar source facts, recursive prepared integer-constant folding,
  future-producer rejection, missing-fact rejection, and mismatched prepared
  producer rejection.

Existing load-local source-fact facade remains
`prepare::find_prepared_same_block_load_local_source_producer`; no AArch64
consumer migration was performed in this packet.

## Suggested Next

Step 3 - Migrate Scalar And Load-Local Availability: update
`dispatch_lookup.cpp` to consume the shared prepared scalar/load-local source
facts and fail closed when facts are absent.

## Watchouts

The new prepared integer-constant query is available for later AArch64
materialization packets, but current AArch64 consumers still call raw
`mir::evaluate_same_block_integer_constant`. Step 3 should avoid expanding
raw scans and should treat absent prepared facts as unavailable rather than
falling back to target-local semantic discovery.

## Proof

`(cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Passed: 169/169 backend tests. Proof log: `test_after.log`.
