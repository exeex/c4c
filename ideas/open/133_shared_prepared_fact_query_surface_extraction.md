# Shared Prepared Fact Query Surface Extraction

## Goal

Extract a shared prepared-fact query surface for target-neutral relationships
that the AArch64 dispatch family still stitches together locally.

## Why This Exists

The post-contract dispatch-family audit found remaining shared-contract gaps
around prepared value homes, edge-publication source producers, current-block
entry publication, and same-block materialization relationships. These facts
should be queried from a shared prepared or prealloc owner rather than
rediscovered independently by AArch64 dispatch-family files.

This idea exists to move query authority, not target-local emission.

## In Scope

- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_lookup.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- The selected shared owner under `src/backend/mir/prealloc/` or
  `src/backend/mir/prepare*`
- Query APIs for prepared value-home, edge-publication source-producer,
  current-block entry publication, and same-block materialization facts

## Out Of Scope

- Moving AArch64 register names, scratch choices, hazard policy, relocation
  operands, or final instruction spelling into shared code
- Replacing prepared facts with predecessor rescans or BIR-name matching
- A broad rewrite of dispatch publication or value materialization
- File-count cleanup unrelated to shared query authority

## Acceptance Criteria

- At least one concrete local AArch64 rediscovery site is replaced by a shared
  prepared/prealloc query.
- The shared API exposes target-neutral fact relationships, not AArch64
  emission policy.
- AArch64 callers preserve register choices, scratch handling, relocation
  operands, and final instruction spelling.
- Nearby same-feature cases are examined so the route is not a one-case query
  wrapper.

## Proof Route

- `cmake --build --preset default`
- `ctest --test-dir build -R '^backend_' --output-on-failure`

Use matching `test_before.log` and `test_after.log` because shared backend
query code is in scope.

## Reviewer Reject Signals

- Target-local AArch64 emission policy moves into shared prepared/prealloc
  code.
- The change reintroduces predecessor rescans, BIR-name matching, or local
  fact reconstruction behind a generic API name.
- The new shared query does not replace an actual local rediscovery site.
- Only one named failing case is proven while nearby same-feature cases are
  unexamined.
- Expectations are weakened or supported backend behavior is marked
  unsupported.
