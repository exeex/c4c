# 202 Phase C2 residual cache contraction audit

## Goal

Audit which residual prepared/prealloc lookup, cache, context, and diagnostic
surfaces can become private or contract after Phase B2 residual schemas exist,
and which must remain public as target policy, pass context, compatibility
adapters, or oracles.

This is Phase C2 of the BIR/prealloc thinning program. It is analysis-only and
must produce follow-up implementation ideas for bounded cache/API contraction.

## Prerequisite

Do not activate this draft until Phase B2 has closed and produced:

- `docs/bir_prealloc_fusion/phase_b2_residual_annotation_schema_candidates.md`

If Phase B2 accepts no schemas, this draft should focus only on retained
target-policy/pass-context classification or be retired.

## Shared Artifact Contract

This phase must read:

- Phase A2 and B2 durable artifacts;
- `docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md`;
- `docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md`;
- `docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md`;
- implementation closure notes for any Phase B2 schema follow-ups;
- current accepted baseline state.

This phase must write its durable analysis payload to:

- `docs/bir_prealloc_fusion/phase_c2_residual_cache_contraction.md`

## Direction

Contraction is allowed only after equivalent BIR facts and consumer/fallback
rules exist. Private-cache contraction must not erase diagnostic/oracle
coverage or move target policy into BIR.

## In Scope

- Residual prepared lookup/cache groups touched by Phase A2/B2 accepted route
  families.
- `PreparedFunctionLookups` groups that Phase B2 schemas make newly
  contractible or newly retained.
- Prepared printer/debug/oracle surfaces that need route-native replacements
  before contraction.
- Pass-local construction caches that can become private after consumers stop
  reading them.
- Follow-up ideas for one cache/API surface at a time.

## Out Of Scope

- Directly deleting prepared APIs in this analysis phase.
- Contracting target-policy surfaces.
- Treating schema existence as consumer migration completion.
- Combining diagnostics, cache contraction, and consumer migration in one
  broad implementation idea.
- Opening draft 155.

## Expected Output

The closure note must contain:

- a link to `docs/bir_prealloc_fusion/phase_c2_residual_cache_contraction.md`;
- cache/API surfaces ready for bounded contraction after consumer migration;
- surfaces that remain public or target-owned;
- diagnostics/oracle replacement prerequisites;
- follow-up implementation ideas with proof recommendations;
- explicit blockers to `PreparedFunctionLookups` and `PreparedBirModule`
  retirement.

## Reviewer Reject Signals

- Deleting caches before equivalent route facts, consumers, and diagnostics are
  ready.
- Hiding prepared fallbacks needed for absent, mismatched, ambiguous, or
  policy-sensitive facts.
- Treating full-suite greenness as cache contraction readiness.
- Moving target-policy or printer/oracle authority into BIR.
- Claiming broad retirement from one selected cache/API surface.
