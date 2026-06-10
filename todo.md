Status: Active
Source Idea Path: ideas/open/166_bir_annotation_lookup_index_schema.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Normalize Route-Specific Lookup Helpers

# Current Packet

## Just Finished

Step 3 completed for `ideas/open/166_bir_annotation_lookup_index_schema.md`:
normalized Route 4 publication availability indexes to the shared
route-reference validation pattern. Route 4 now has current-block and
block-entry publication reference validators using the shared route/category,
owner-scope, stable BIR key, relationship, and record-index metadata while
keeping semantic payloads in the typed Route 4 records. The validators preserve
existing public lookup answer shapes and fail closed for missing records,
stale function/block ownership, wrong keys/type mismatches, wrong
relationships through value-link scope, duplicate index references, no-match,
and record-pointer divergence.

## Suggested Next

Execute Step 4: add the private aggregate facade or next normalization slice
only if the active plan calls for it. Keep the facade non-durable and limited
to typed Route index/reference handles; do not turn it into a BIR-owned
`PreparedFunctionLookups` clone.

Delegated Step 3 proof command:
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

## Proof

Proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$' > test_after.log`

Result: passed. `test_after.log` reports
`1/1 Test #117: backend_prepared_lookup_helper ... Passed` and
`100% tests passed, 0 tests failed out of 1`.

Additional validation: `git diff --check`.
