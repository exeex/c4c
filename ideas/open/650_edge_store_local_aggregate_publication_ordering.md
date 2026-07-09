# Edge-Store Local Aggregate Publication Ordering

Status: Open
Type: Implementation
Parent: `ideas/closed/640_mixed_local_global_publication_authority.md`
Related:
- `ideas/closed/640_mixed_local_global_publication_authority.md`
- `ideas/open/641_aggregate_global_object_materialization_policy.md`
- `ideas/closed/631_direct_global_symbol_local_memory_policy.md`
Owning Layer: prepared publication ordering for edge-store local destinations
and local aggregate/frame-slot reads
Queue Order: 50
Prerequisites: edge-store destination identity, predecessor ordering,
local-slot or aggregate lane identity, and source freshness must be explicit
before RV64 consumes the value.
Proof Surface: `src/pr68185.c` and `src/pr68321.c` after scalar frame-slot
local-memory lookup is no longer the first owner.

## Goal

Define the explicit publication-ordering authority for local values that are
published through edge-store-slot destinations or local aggregate/frame-slot
reads before RV64 consumes them.

## Why This Exists

Idea 640 classified `src/pr68185.c` and `src/pr68321.c` as outside the shared
scalar frame-slot local-memory repair. The remaining evidence points at
edge-store-slot local destination publication and local aggregate/frame-slot
ownership, not direct global-symbol local-memory or the scalar lookup family
that idea 640 closed.

## In Scope

- Refresh `src/pr68185.c`, `src/pr68321.c`, and nearby edge-store local
  publication-ordering residuals.
- Identify the predecessor edge, edge-store destination, local-slot or
  aggregate lane, source value, and consumer point that require explicit
  ordering authority.
- Add producer or RV64 consumer support only when the selected edge-store or
  local aggregate publication authority is explicit.
- Preserve fail-closed diagnostics for ambiguous predecessor order, missing
  destination ownership, incomplete aggregate/lane identity, stale source
  values, and scalar-only frame-slot facts.

## Out Of Scope

- Scalar frame-slot local-memory lookup repaired by idea `640`.
- Aggregate global-object materialization owned by idea `641` unless refreshed
  evidence proves a global-object materialization boundary.
- Aggregate/sret/byval stack-home policy owned by idea `633`.
- Move-bundle fan-in authority, direct global-symbol local memory, ABI/runtime
  policy, expectations, unsupported markers, allowlists, timeouts, or
  accounting.

## Acceptance Criteria

- A refreshed probe proves whether the selected rows share an edge-store local
  publication-ordering family or should be split again.
- At least one complete-authority edge-store/local aggregate publication shape
  moves past its current owner, or the route records the exact missing
  producer or RV64 consumer authority that blocks it.
- Negative proof keeps ambiguous order, missing destination ownership,
  aggregate-lane mismatches, and scalar-only frame-slot cases fail-closed.

## Reviewer Reject Signals

- Reject named-case fixes for `src/pr68185.c`, `src/pr68321.c`, `%t38.phi`,
  `%t17.phi`, or local array `g` without a semantic edge-store or local
  aggregate publication-ordering rule.
- Reject treating edge-store-slot or local aggregate publication as scalar
  frame-slot lookup repaired by idea `640`.
- Reject inferring publication order from source statement order, final
  assembly order, diagnostic wording, block labels, or testcase identity.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime, or
  accounting changes as capability progress.
- Reject helper renames or diagnostic wording changes that leave the same
  missing publication-ordering authority behind a new label.
