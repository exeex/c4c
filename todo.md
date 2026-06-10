Status: Active
Source Idea Path: ideas/open/166_bir_annotation_lookup_index_schema.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Shared Index Reference Contracts

# Current Packet

## Just Finished

Step 2 completed for `ideas/open/166_bir_annotation_lookup_index_schema.md`:
defined the first shared index reference/validation contract and applied it to
Route 7 comparison/condition indexes. The new reference metadata identifies the
route, record category, owner scope, stable BIR keys, relationship kind, and
typed Route 7 record pointer without copying prepared payloads or target
policy. Route 7 validation helpers now fail closed for stale block owners,
missing records, wrong operand relationships, wrong keys, duplicate references,
absent provenance/no-match statuses, and record/key divergence.

## Suggested Next

Execute Step 3: extend the shared reference/validation pattern beyond the
Route 7 first target, preferably by adding the next narrow route index contract
while keeping the existing typed Route records as the semantic payload owners.
Keep any aggregate facade private and rebuildable from route-specific indexes.

Delegated Step 2 proof command:
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
- Route 7 is the only route with the shared reference contract so far. Other
  route indexes still store rebuilt record snapshots without the new validation
  facade.
- The Route 7 contract intentionally validates function/block ownership and
  record pointers; copied blocks with matching labels are rejected as stale
  owners for the block-backed path.

## Proof

Proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$' > test_after.log`

Result: passed. `test_after.log` reports
`1/1 Test #117: backend_prepared_lookup_helper ... Passed` and
`100% tests passed, 0 tests failed out of 1`.

Additional validation: `git diff --check`.
