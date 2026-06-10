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

## Completion Note

Closed on 2026-06-10.

The runbook selected AArch64 scalar value-publication select-chain
materialization in
`src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp` as the one
bounded consumer family. That path now reads Route 2-backed
`mir::find_bir_select_chain_identity(...)` for select root identity, root
instruction index, scalar materialization eligibility, and direct-global
dependency presence instead of using prepared select-chain/direct-global lookup
fields as its primary semantic source.

Oracle coverage was confirmed and extended for direct-global positive behavior
and fail-closed non-select-root, no-dependency, mismatched-type, missing-root,
indexed-record, and scalar-ineligible cases. Prepared answers remained visible
as the comparison oracle while the selected consumer migrated.

No prepared select-chain/direct-global/scalar-materialization API or cache
surface was contracted. The Step 4 consumer scan found remaining production
consumers in publication/store planning, call planning, AArch64 calls, memory,
ALU, FP, producer, edge-copy paths, and prepared printer/test-facing surfaces.
Those residual consumers are follow-up evidence for future Route 2 thinning
ideas, not blockers for this idea, because this source idea was scoped to one
selected consumer family and only allowed contraction when that surface was no
longer needed.

Close-time regression guard passed using existing `test_before.log` and
`test_after.log` in non-decreasing mode. Both logs covered
`backend_prepared_lookup_helper` and `backend_aarch64_instruction_dispatch`,
with 2 passed and 0 failed before and after.
