Status: Active
Source Idea Path: ideas/open/166_bir_annotation_lookup_index_schema.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Broaden Validation And Prepare Closure

# Current Packet

## Just Finished

Step 5 completed for `ideas/open/166_bir_annotation_lookup_index_schema.md`:
broadened validation from the focused Step 4 proof to the full backend CTest
scope and prepared closure readiness notes. The broad backend after-run matched
the delegated `^backend_` scope and passed 179/179 tests.

## Suggested Next

Supervisor can run the regression guard against the existing matching
`test_before.log` baseline and decide whether to close or archive the active
plan.

Delegated Step 5 proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Also run `git diff --check`.

## Watchouts

- Closure readiness: Route 4 and Route 7 have shared reference/validation
  contracts, and the private `RouteIndexReferenceFacade` remains a
  non-durable typed-index handle bundle.
- The accepted route did not create a durable BIR-owned
  `PreparedFunctionLookups` clone.
- The accepted route did not move semantic payloads into indexes or the facade;
  typed Route 4/Route 7 records remain the semantic payload owners.
- The accepted route did not import target, ABI, layout, register allocation,
  move scheduling, call plan, memory-addressing, frame, dynamic-stack, helper,
  final-emission, storage-home, condition-code, or target lowering policy into
  the BIR index/facade schema.
- The accepted route did not switch target, codegen, or prealloc production
  consumers.
- No expectation downgrades or helper-only reshuffles were accepted; the facade
  is exercised through focused Route 4/Route 7 validation and a migrated
  low-risk BIR materialized-condition helper.
- Residual risk: Route 1, Route 2, Route 3, Route 5, and Route 6 still use
  rebuilt record snapshot indexes without the shared reference/validation
  facade. Route 4 validation intentionally treats copied blocks outside the
  indexed function as stale, even though legacy public lookup helpers may still
  match some external blocks by stable label/id.
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
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: passed. `test_after.log` reports
`100% tests passed, 0 tests failed out of 179`; backend label summary reports
`backend = 15.15 sec*proc (179 tests)`.

Additional validation: `git diff --check`.
