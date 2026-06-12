# 218 Phase E1 semantic duplicate candidate triage

## Goal

Turn the Phase E0 semantic-duplicate candidate set into narrowly scoped
follow-up ideas, one duplicate semantic helper family or one row-scoped
semantic/oracle surface at a time.

This is Phase E1 of the BIR/prealloc thinning program. It is analysis-only for
this activation. It must not directly migrate helpers, delete prepared APIs,
move target policy into BIR, or open broad `PreparedBirModule` /
`PreparedFunctionLookups` retirement work.

## Why This Exists

Phase E0 closed in
`ideas/closed/217_phase_e0_prepared_bir_merge_readiness_and_ownership_boundary_audit.md`.
Its durable artifact,
`docs/bir_prealloc_fusion/phase_e0_prepared_bir_merge_readiness.md`, establishes
the target end state:

- BIR owns semantics.
- Prepared owns only policy.
- Prepared APIs become private pass context.
- Duplicate semantic helpers are the primary code-deletion target.

E0 also states that later E1-E5 work should be opened as separately scoped
initiatives: one semantic duplicate family, public prepared API contraction,
diagnostic/oracle row, cross-target wrapper boundary, or Route 8 owner/schema
analysis at a time. E1 therefore starts by selecting and drafting the next
safe semantic-duplicate follow-ups rather than implementing all E1 candidates
in one broad plan.

## Required Inputs

This phase must read:

- `ideas/closed/217_phase_e0_prepared_bir_merge_readiness_and_ownership_boundary_audit.md`
- `docs/bir_prealloc_fusion/phase_e0_prepared_bir_merge_readiness.md`
- `ideas/closed/181_phase_d_mir_consumer_bir_view_switch_plan.md`
- `ideas/closed/203_phase_d2_retained_surface_consumer_switch_analysis.md`
- `docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md`
- `docs/bir_prealloc_fusion/phase_d2_retained_surface_consumer_switch_analysis.md`
- `docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md`
- the Route 3 through Route 7 closure notes `207` through `216`
- `src/backend/prealloc/module.hpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- current accepted baseline state.

This phase must write its durable analysis payload to:

- `docs/bir_prealloc_fusion/phase_e1_semantic_duplicate_candidate_triage.md`

## Direction

E1 must classify the E0 semantic duplicate candidates into follow-up
readiness buckets:

- ready to draft one implementation idea;
- needs more analysis before implementation;
- defer to E2 because the real work is public API/private pass-context
  contraction;
- defer to E3 because the real work is diagnostic/oracle replacement;
- defer to E4 because the real work is cross-target wrapper convergence;
- blocked because the surface is target/prepared policy, fallback/oracle, or
  aggregate compatibility.

The accepted E1 candidate families from E0 are:

- aggregate BIR semantic forwarding;
- control-flow identity helpers;
- route source/publication/join/call/comparison identity helpers;
- liveness identity helpers;
- intrinsic metadata adapters;
- row-scoped diagnostic/oracle helpers.

For each candidate family, E1 must identify the exact helper/API family, the
BIR fact or annotation that would own the semantics, the prepared fallback or
policy that must remain, the public consumers still present, and the proof
shape needed before code deletion or helper contraction is allowed.

## In Scope

- Semantic duplicate helper families only.
- BIR-owned target-neutral facts, route facts, indexes, identities, and
  annotations as replacement authority.
- Prepared fallback and target/prepared policy boundaries that must remain.
- Code-size-positive deletion or contraction opportunities that remove
  duplicate semantic authority rather than renaming wrappers.
- Follow-up ideas for one helper family or one semantic/oracle row at a time.

## Out Of Scope

- Direct implementation.
- Direct helper deletion or prepared API privatization.
- Aggregate `PreparedBirModule` or `PreparedFunctionLookups` retirement.
- Public prepared API contraction; that belongs to E2 after a selected E1
  semantic replacement is proven.
- Broad prepared printer/debug/oracle replacement; that belongs to E3.
- Cross-target wrapper migration; that belongs to E4.
- Route 8 return-chain owner/schema design, except as a deferred separate
  analysis line.
- Moving ABI, layout, registers, stack, address materialization, move
  scheduling, branch spelling, helper protocols, final records, target-wrapper
  behavior, or emitted output into target-neutral BIR.
- Weakening tests, diagnostics, baselines, helper-oracle names, supported-path
  status, or expected strings.

## Expected Output

The closure note must contain:

- a link to
  `docs/bir_prealloc_fusion/phase_e1_semantic_duplicate_candidate_triage.md`;
- a candidate-by-candidate triage table for the six E0 E1 families;
- accepted follow-up ideas, each scoped to exactly one semantic duplicate
  helper family or one row-scoped semantic/oracle surface;
- explicit deferrals to E2, E3, E4, or Route 8 analysis where E1 is not the
  right owner;
- no-action decisions for aggregate, target-policy, fallback/oracle,
  diagnostic/string, and cross-target compatibility surfaces;
- the proof requirements for any accepted implementation idea, including
  positive, absent, invalid, duplicate/conflict, mismatch, fallback,
  printer/debug, wrapper, and expected-string behavior where applicable;
- an explicit statement that draft 155 / E5 remains unopened.

## Reviewer Reject Signals

- Treating E0's candidate categories as implementation permission.
- Opening a broad migration plan that covers multiple helper families.
- Counting facade renames, wrapper moves, or container reshuffles as code
  reduction.
- Moving target/prepared policy into BIR.
- Deleting prepared fallback/oracle surfaces before equivalent proof exists.
- Weakening diagnostics, helper-oracle names, supported-path status, baselines,
  or expected strings.
- Opening draft 155 or claiming broad prepared retirement from E1 triage.
