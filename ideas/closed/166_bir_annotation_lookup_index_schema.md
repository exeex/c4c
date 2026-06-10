# 166 BIR annotation lookup index schema

## Goal

Prototype function-local lookup indexes over BIR annotation records without
turning those indexes into a function-level lowering-plan container.

## Why This Exists

Phase B found that every accepted route needs cheap lookup by stable BIR ids:
function, block label, instruction index, value/name, call-site ordinal, edge
key, before-instruction index, or relationship kind. The durable payloads
belong on value, instruction, terminator, block, or edge annotations, but
function-level indexes are needed to preserve prepared-query lookup cost during
migration.

## In Scope

- Define function-local indexes that reference typed annotation records for the
  Phase A Route 1 through Route 7 domains.
- Preserve current prepared-query lookup shapes where they are performance
  critical, while making the BIR annotation record the source of truth.
- Specify rebuild or validation expectations so index entries cannot diverge
  from annotation payloads.
- Keep aggregate facades private while consumers switch one relationship at a
  time.

## Out Of Scope

- A durable `PreparedFunctionLookups` clone in BIR.
- Function-level ABI, layout, register allocation, move scheduling, call plan,
  memory-addressing, frame, dynamic-stack, helper, or final-emission metadata.
- Duplicating semantic payloads in indexes when typed annotations already own
  those payloads.

## Acceptance Criteria

- Function indexes point at typed annotation payloads and can be rebuilt or
  validated from those payloads.
- Consumers retain cheap lookup identity without expensive scans and without
  adding a new lowering-plan container to BIR.
- Index contracts cover the route-specific lookup keys required by Phase B
  without inventing new semantic routes.

## Reviewer Reject Signals

- Reject if the index schema stores full semantic payloads that should live on
  value, instruction, terminator, block, or edge annotations.
- Reject if the route creates a BIR-owned `PreparedFunctionLookups` aggregate
  containing call plans, memory plans, move bundles, return-chain caches,
  storage homes, ABI placement, or target lowering policy.
- Reject if lookup performance is claimed while consumers still scan broad BIR
  structures or silently fall back to prepared-only answers.
- Reject if a helper rename or facade split is claimed as progress without
  typed annotation references and divergence checks.
- Reject if the schema introduces a new Phase A semantic route instead of
  indexing the already accepted Route 1 through Route 7 relationships.

## Closure Notes

Closed after implementing a Route index reference contract, Route 4 and Route
7 reference validation, a private `RouteIndexReferenceFacade`, focused
fail-closed validation coverage, and a low-risk materialized-condition helper
migration through the facade.

Reviewer report `review/166_bir_annotation_lookup_index_route_review.md` found
no blocking issues and recommended closure. The accepted residual risk is that
Route 1, Route 2, Route 3, Route 5, and Route 6 retain existing typed
record/index shapes rather than the shared reference facade. This is acceptable
for prototype closure because the route proved typed-record references and a
facade migration without creating a durable lowering-plan aggregate.

Closure validation used canonical broad backend logs from:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

`test_before.log` and `test_after.log` both passed `179/179`, and the
regression guard passed with no new failures or pass-count regression.
