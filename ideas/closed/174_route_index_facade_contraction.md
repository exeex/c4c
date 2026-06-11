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

## Closure Note

Closed on 2026-06-11 after the selected Route 4 block-entry facade contraction
runbook completed.

The completed route selected the block-entry publication category, tightened
typed fail-closed validation, migrated the selected consumer boundary through
the facade validator, and removed the redundant public
`route4_find_block_entry_publication` direct lookup surface.

Acceptance proof passed for:

```bash
cmake --build build --target backend_prepared_lookup_helper_test backend_prealloc_block_entry_publications_test
ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_prealloc_block_entry_publications)$' --output-on-failure
```

Close-time regression guard passed with canonical `test_before.log` and
`test_after.log` using non-decreasing pass-count mode: 2 passed before, 2
passed after, 0 failures before, 0 failures after.

No generic lowering-plan aggregate, broad BIR scan, semantic payload copy, or
unsupported expectation downgrade was introduced. Any future facade category or
consumer migration should start as a separate idea with its own typed validator
and consumer boundary.

## Reviewer Reject Signals

- A generic facade becomes a new lowering-plan container.
- Consumers scan broad BIR structures instead of validated typed references.
- Helper renaming or facade reshaping is claimed as cache contraction.
