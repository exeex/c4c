Status: Active
Source Idea Path: ideas/open/166_bir_annotation_lookup_index_schema.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate A Low-Risk Consumer Facade

# Current Packet

## Just Finished

Step 4 completed for `ideas/open/166_bir_annotation_lookup_index_schema.md`:
added a non-durable `RouteIndexReferenceFacade` over existing Route 4 and Route
7 typed index handles and migrated the low-risk BIR
`find_materialized_condition_producer_identity(...)` helper through the Route 7
facade validation path. The facade stores only pointers to typed route indexes
and delegates to existing Route 4/Route 7 reference validators; it does not
store semantic payloads or target/prealloc policy.

## Suggested Next

Execute Step 5: broaden backend validation and prepare closure if the active
plan is otherwise complete. Confirm that the facade remains non-durable,
typed-index-only, and not a `PreparedFunctionLookups` clone.

Delegated Step 4 proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$' > test_after.log`

Also run `git diff --check`.

## Watchouts

- Keep typed Route 1 through Route 7 annotation records as the semantic source
  of truth.
- Do not create a durable BIR-owned `PreparedFunctionLookups` clone.
- Do not add ABI, layout, register allocation, move scheduling, call plan,
  memory-addressing, frame, dynamic-stack, helper, final-emission,
  storage-home, or target lowering policy data to function-local indexes.
- Do not duplicate semantic payload fields in indexes when typed annotations
  already own those payloads.
- Preserve explicit divergence, stale-reference, duplicate-key, missing-record,
  wrong-relationship, and no-match cases where applicable.
- Route 4 and Route 7 now have shared reference/validation contracts. Other
  route indexes still store rebuilt record snapshots without the validation
  facade.
- The Route 7 contract intentionally validates function/block ownership and
  record pointers; copied blocks with matching labels are rejected as stale
  owners for the block-backed path.
- The Route 4 contract intentionally treats copied blocks outside the indexed
  function as stale for validation, even though legacy public lookup helpers
  may still match some external blocks by stable label/id.
- The new facade is intentionally a thin local handle bundle over Route 4 and
  Route 7 indexes. It should not grow semantic payload vectors or import target
  policy in later packets.

## Proof

Proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$' > test_after.log`

Result: passed. `test_after.log` reports
`1/1 Test #117: backend_prepared_lookup_helper ... Passed` and
`100% tests passed, 0 tests failed out of 1`.

Additional validation: `git diff --check`.
