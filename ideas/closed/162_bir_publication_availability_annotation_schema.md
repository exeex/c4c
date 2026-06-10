# 162 BIR publication availability annotation schema

## Goal

Prototype BIR block/value annotations for block-entry and current-block
publication identity established by Phase A Route 4.

## Why This Exists

Phase A proved current-block and block-entry value availability, source
producer identity, produced BIR value/name, producer instruction/index, and
source-producer kind. Phase B classified availability as block-scoped schema
with value annotations for the source and produced-value payload, while
rejecting publication mechanics.

## In Scope

- Define block annotations for value availability at block entry and current
  program points.
- Define value annotation payloads for produced/consumed value identity, source
  producer identity, and producer kind references.
- Keep current prepared publication reads as migration oracles until BIR-backed
  availability answers are proven equivalent.
- Preserve cheap availability lookup without turning the function index into a
  lowering-plan container.

## Out Of Scope

- Hook kind, destination home, storage encoding, stack-source extension policy,
  register-view conversion, immediate publication payload spelling, emitted
  storage availability, and scalar publication emission policy.
- Whole prepared publication plan records as BIR schema.

## Acceptance Criteria

- BIR-owned block/value annotations answer semantic availability and
  source-producer identity without exposing storage or publication emission
  choices.
- Existing current-block and block-entry prepared reads remain comparison
  oracles during migration.
- Unavailable cases remain represented explicitly enough for consumers to avoid
  falling back to storage-policy inference.

## Reviewer Reject Signals

- Reject if the schema imports publication hooks, destination homes, register
  views, stack-source extension policy, immediate payload spelling, emitted
  storage availability, or scalar publication emission order.
- Reject if `PreparedCurrentBlockPublicationConsumption` or publication plan
  records are copied wholesale into BIR.
- Reject if only current-block available cases are proven while block-entry,
  unavailable, and alternate source-producer cases remain prepared-only.
- Reject if lookup performance is preserved by a new function-level lowering
  plan rather than typed indexes over block/value annotations.
- Reject broad rewrites of scalar publication emission as schema progress.

## Closure Notes

Closed after implementing Route 4 BIR publication availability block/value
records, BIR-owned lookup/index helpers, oracle tests for current-block and
block-entry availability, and a low-risk current-block MIR query migration.

Reviewer report `review/162_publication_availability_route_review.md` found no
blocking issues and judged closure reasonable. The only residual scope is that
the block-entry MIR consumer remains on its older semantic PHI scan while Route
4 block-entry records/indexes are implemented and oracle-covered; this was
accepted because the source idea required BIR-owned schema and migration
oracles, not every block-entry consumer migration in this slice.

Closure validation used canonical broad backend logs from:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

`test_before.log` and `test_after.log` both passed `179/179`, and the
regression guard passed with no new failures or pass-count regression.
