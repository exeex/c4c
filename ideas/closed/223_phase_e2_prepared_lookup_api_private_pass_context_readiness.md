# 223 Phase E2 prepared lookup API private/pass-context readiness

## Goal

Analyze which public prepared lookup APIs can be downgraded toward private
codegen pass context after Phase E1 proved two selected semantic identity
helper contractions, and produce follow-up implementation ideas for exactly
one public API boundary at a time.

This is Phase E2 of the BIR/prealloc thinning program. This activation is
analysis-only. It must not directly delete prepared APIs, privatize aggregate
lookup construction, or claim `PreparedFunctionLookups` retirement.

## Why This Exists

Phase E0 defined the target end state:

- BIR owns semantics.
- Prepared owns only policy.
- Prepared APIs become private pass context.
- Duplicate semantic helpers are the primary code-deletion target.

Phase E1 triage opened two selected semantic-duplicate implementation ideas,
and both have closed:

- `ideas/closed/219_phase_e1_control_flow_identity_helper_contraction.md`
- `ideas/closed/220_phase_e1_route_identity_helper_contraction.md`

Those closures prove one control-flow branch-target identity helper and one
Route 7 fused-compare operand producer identity helper can read BIR/route
identity under agreement while preserving prepared fallback and target policy.
They do not prove that whole lookup groups, aggregate construction, or public
prepared fallback/oracle APIs can be deleted. E2 must decide what, if anything,
is now ready to become private pass context.

## Required Inputs

This phase must read:

- `ideas/closed/217_phase_e0_prepared_bir_merge_readiness_and_ownership_boundary_audit.md`
- `docs/bir_prealloc_fusion/phase_e0_prepared_bir_merge_readiness.md`
- `ideas/closed/218_phase_e1_semantic_duplicate_candidate_triage.md`
- `docs/bir_prealloc_fusion/phase_e1_semantic_duplicate_candidate_triage.md`
- `ideas/closed/219_phase_e1_control_flow_identity_helper_contraction.md`
- `ideas/closed/220_phase_e1_route_identity_helper_contraction.md`
- `docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md`
- `docs/bir_prealloc_fusion/phase_d2_retained_surface_consumer_switch_analysis.md`
- `src/backend/prealloc/module.hpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- current accepted baseline state and hook review state.

This phase must write its durable analysis payload to:

- `docs/bir_prealloc_fusion/phase_e2_prepared_lookup_api_private_pass_context_readiness.md`

## Direction

E2 decides public API/private-pass-context readiness. It does not implement the
contractions itself.

For each candidate API or lookup group, classify it as one of:

- ready to draft one public API contraction implementation idea;
- private pass-context candidate only after another named E1 semantic helper
  contraction;
- retained public fallback/oracle;
- retained target/prepared policy;
- diagnostic/oracle prerequisite owned by E3;
- cross-target wrapper prerequisite owned by E4;
- blocked aggregate compatibility surface;
- no-action.

Accepted E2 follow-up ideas must name exactly one public prepared API boundary
or one helper family, the BIR/route semantic owner already proven, the
prepared fallback/policy/oracle that remains, every public consumer still
present, and the proof required before privatization or deletion can be
claimed.

## Candidate Surfaces

Start from the E0/E1 E2 candidate set:

- selected `call_plans` Route 6 call-use source adapter paths;
- selected `memory_accesses` Route 3 memory/source adapter paths;
- selected `edge_publications` and `edge_publication_source_producers` Route 4
  or Route 5 publication/edge/join adapter paths;
- selected Route 1/2/5/6/7 source-producer helper or printer/oracle rows;
- identity-only `move_bundles` or `value_homes` audit candidates;
- the newly proven E1 selected helpers:
  `find_prepared_control_flow_branch_target_labels(...)` and
  `find_prepared_fused_compare_operand_producer_facts(...)`.

## In Scope

- One public prepared API boundary or helper family at a time.
- Whether a proven E1 semantic identity path allows the prepared API to become
  private pass context.
- Consumer inventory for production, printer/debug, target-wrapper, oracle,
  fallback, and expected-string users.
- Follow-up implementation ideas for safe one-boundary contraction.
- Explicit blockers for APIs that must remain public.

## Out Of Scope

- Direct implementation.
- Direct helper deletion or privatization.
- Aggregate `PreparedFunctionLookups` deletion, broad privatization, facade
  rename, or construction removal.
- `PreparedBirModule` retirement, demotion, or draft 155 / E5 work.
- Moving target ABI/layout/register/stack/address/move/call/branch/emission
  policy into BIR.
- Broad prepared printer/debug/oracle replacement; that belongs to E3.
- Cross-target wrapper migration; that belongs to E4.
- Baseline refreshes, expected-string rewrites, helper renames, unsupported
  downgrades, or timeout masking as proof.

## Expected Output

The closure note must contain:

- a link to
  `docs/bir_prealloc_fusion/phase_e2_prepared_lookup_api_private_pass_context_readiness.md`;
- a candidate-by-candidate readiness table for the E2 surfaces above;
- any accepted follow-up implementation ideas, each scoped to exactly one
  public prepared API boundary or helper family;
- explicit no-action decisions for aggregate lookup construction and
  target-policy/fallback/oracle surfaces;
- deferrals to E1, E3, E4, Route 8, or E5 where E2 is not the correct owner;
- proof requirements for accepted implementation ideas, including positive,
  absent, invalid, duplicate/conflict, mismatch, fallback, printer/debug,
  wrapper, and expected-string behavior where applicable;
- an explicit statement that draft 155 / E5 and aggregate
  `PreparedFunctionLookups` retirement remain unopened.

## Reviewer Reject Signals

- Treating the completed E1 helper slices as proof that a whole lookup group
  can be deleted.
- Hiding aggregate `PreparedFunctionLookups` behind a new private facade while
  public consumers still exist.
- Privatizing a helper while prepared fallback, printer/debug, wrapper, or
  oracle users still need the public surface.
- Moving target/prepared policy into BIR or route authority.
- Counting facade renames, wrapper moves, or construction reshuffles as code
  reduction.
- Weakening diagnostics, helper-oracle names, supported-path status, baselines,
  or expected strings.
- Opening draft 155 or claiming broad prepared retirement from E2 readiness.

## Closure Note

Closed after the analysis-only E2 runbook completed. The durable output is
`docs/bir_prealloc_fusion/phase_e2_prepared_lookup_api_private_pass_context_readiness.md`.

The E2 readiness review accepted exactly two scoped follow-up ideas:

- `ideas/open/224_phase_e2_control_flow_branch_target_helper_private_pass_context.md`
- `ideas/open/225_phase_e2_fused_compare_operand_producer_helper_private_pass_context.md`

All other candidate surfaces remain governed by the E2 document's retained,
deferred, or no-action decisions. Draft 155 / E5 and aggregate
`PreparedFunctionLookups` retirement remain unopened.

Closure proof used the existing matching `test_before.log` and
`test_after.log` two-test backend subset with the regression guard's
non-decreasing maintenance mode: 2/2 passing before and after, with no new
failures.
