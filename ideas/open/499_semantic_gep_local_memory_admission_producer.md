# BIR Semantic GEP Local-Memory Admission Producer

Status: Open
Type: Focused BIR semantic producer implementation idea
Parent: `ideas/closed/496_semantic_lir_to_bir_admission_high_impact_cleanup.md`
Source Evidence:
- `build/agent_state/496_step1_semantic_admission_after_scalar_local_loads/summary.md`
- `build/agent_state/496_step1_semantic_admission_after_scalar_local_loads/semantic_family_counts.tsv`
- `build/agent_state/496_step1_semantic_admission_after_scalar_local_loads/semantic_family_representatives.tsv`
- `build/agent_state/496_step2_next_owner_selection/decision.md`
- `build/agent_state/496_step2_next_owner_selection/open_idea_inventory.tsv`
Owning Layer: BIR semantic GEP/local-memory admission producer

## Goal

Classify and repair the `62`-row semantic `gep local-memory` admission family
without moving address/provenance producer responsibility into RV64/MIR
lowering.

## Why This Exists

Idea 496 refreshed semantic admission after `local_array_scalar_local_loads`
became available. The largest remaining producer-owned family is
`gep local-memory semantic family` with `62` rows, represented by
`src/pr44468.c`, `src/pr48571-1.c`, `src/pr65956.c`, `src/pr80421.c`, and
`src/20000717-4.c`.

Step 2 found no existing open idea that is a clean, unblocked match. The related
pointer-value provenance idea is parked on closed-world formal-pointer authority
and is scoped to runtime pointer dereferences, not semantic local GEP admission.

## In Scope

- Reproduce the `gep local-memory semantic family` rows from the current
  semantic admission evidence.
- Classify representatives by source object, local address/provenance
  authority, element path, offset/layout, aggregate/member boundary, pointer
  provenance boundary, variadic/runtime boundary, and target-only evidence.
- Define the BIR semantic admission contract for local-memory GEP records that
  have explicit producer authority.
- Implement the narrow producer surface only where source object, address
  derivation, element path, layout/range, and provenance authority are explicit.
- Preserve fail-closed statuses for missing or incoherent authority.

## Out Of Scope

- RV64/MIR lowering that reconstructs missing GEP authority from raw LIR/BIR
  shape, final homes, names, testcase shape, or target behavior.
- Reopening scalar local-load, local-address provenance, checker-input,
  proof-fact, range-proof, or selected-path producers unless a concrete
  regression proves their handback wrong.
- Store local-memory, direct-call, scalar/local-memory, memcpy/memset, alloca,
  signature, control-flow, F128, move-bundle, runtime mismatch, global-data, or
  stack-frame work.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline churn.

## Acceptance Criteria

- The 62-row `gep local-memory` family is classified into explicit producer
  subfamilies with representatives and fail-closed reasons.
- Available semantic GEP admission requires explicit source object,
  local-address/provenance, element path, offset/layout, and range authority.
- Missing source object, missing derivation, missing provenance, aggregate or
  member boundary, pointer-derived runtime provenance, variadic/runtime,
  unsupported type, raw-shape-only, target-only, and coordinate-confusion cases
  remain distinguishable and fail closed.
- Any downstream RV64/MIR work is sequenced only after producer facts close with
  proof.

## Reviewer Reject Signals

Reject this idea as progress if the change:

- implements semantic GEP admission by matching testcase names, value names,
  dump order, route-local slots, lowered index values, final homes, or RV64
  target behavior;
- reconstructs source object, element offset, layout, range, or provenance in
  RV64/MIR lowering instead of publishing BIR semantic authority;
- weakens unavailable statuses or treats raw shape as available authority;
- folds store, call, runtime, F128, move-bundle, stack-frame, scalar-load, or
  broad generic local-memory work into this focused packet;
- changes expectations, unsupported markers, allowlists, pass/fail accounting,
  runtime comparison, or baseline logs as proof of progress.
