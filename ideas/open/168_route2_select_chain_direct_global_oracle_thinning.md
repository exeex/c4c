# 168 Route 2 select-chain/direct-global oracle thinning

## Goal

Migrate one selected select-chain/direct-global consumer group to Route 2 BIR
records and contract only the prepared cache/API surface no longer needed by
that consumer.

## Source

This idea comes from Phase C:

- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- Route 2 row under `## Step 4 Follow-Up Ideas And Proof Routes`

## Scope

- Select-chain value/dependency lookup caches.
- Scalar materialization eligibility lookup.
- Direct-global dependency indexes layered through select-chain/source-producer
  lookup fields.
- One consumer family from call, publication, materialization, or
  source-producer paths.

## Out Of Scope

- Materialization cost, hazard decisions, register availability, publication
  routing, call ABI behavior, and final move/branch choices.
- Deleting prepared select-chain helpers while unaudited consumers still depend
  on them.

## Acceptance Criteria

- The selected consumer reads `Route2SelectChainValueIndex` or an equivalent
  BIR-backed facade.
- Prepared answers remain oracle-visible until positive and negative
  equivalence is proven.
- Direct-global, non-select-root, no-dependency, and scalar-ineligible cases are
  covered.

## Proof Route

Use oracle tests for direct-global positive cases plus non-select-root,
no-dependency, and scalar-ineligible rejects. Pair those with the narrow
consumer subset that exercises the migrated call/publication/materialization
path.

## Reviewer Reject Signals

- Turning select-chain identity into a call-specific or AArch64-specific
  shortcut.
- Moving target materialization or register policy into BIR.
- Proving only a single named direct-global success case.
