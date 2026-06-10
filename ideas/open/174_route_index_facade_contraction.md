# 174 Route index facade contraction

## Goal

Expand or contract the route-index reference facade only where a route has
explicit typed reference validation and a selected consumer boundary that can
use the facade without creating a new lowering-plan aggregate.

## Source

This idea comes from Phase C:

- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- Route index facade row under `## Step 4 Follow-Up Ideas And Proof Routes`

## Scope

- Selected reference-validation internals for routes with typed validators.
- Existing Route 4 and Route 7 facade coverage.
- One new route-specific reference category only if the typed route records
  remain the semantic owner.

## Out Of Scope

- Replacing typed route indexes wholesale.
- Creating a BIR-owned `PreparedFunctionLookups` clone.
- Duplicating semantic payloads in facade records.
- Routing broad consumer migrations through a generic facade without typed
  route validation.

## Acceptance Criteria

- Any new facade category has fail-closed typed reference validation.
- The facade stores references, not semantic payload copies.
- The selected consumer uses the facade only after route-specific validation is
  proven.

## Proof Route

Prove fail-closed validation for each newly supported reference category, then
run the exact migrated consumer subset. Broad aggregate contraction is not
accepted from facade tests alone.

## Reviewer Reject Signals

- A generic facade becomes a new lowering-plan container.
- Consumers scan broad BIR structures instead of validated typed references.
- Helper renaming or facade reshaping is claimed as cache contraction.
