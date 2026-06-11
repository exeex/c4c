# 202 Phase C2 selected adapter cache contraction readiness audit

## Goal

Audit whether the Phase B2 selected adapters and diagnostic/oracle follow-ups
make any prepared/prealloc lookup, cache, context, diagnostic, or API surface
ready for bounded contraction, or whether they instead prove that the surface
must remain public as fallback, oracle, target policy, or pass context.

This is Phase C2 of the BIR/prealloc thinning program. It is analysis-only and
must produce follow-up implementation ideas only for surfaces that are truly
ready for bounded cache/API contraction.

## Why This Exists

Phase B2 closed in
`ideas/closed/201_phase_b2_selected_route_extension_schema_readiness_audit.md`.
Its durable artifact found no accepted schema/index extension work. Instead,
it opened bounded follow-ups:

- `ideas/closed/202_route3_memory_source_identity_adapter.md`
- `ideas/closed/203_route4_publication_identity_adapter.md`
- `ideas/closed/204_route5_edge_join_source_adapter.md`
- `ideas/closed/205_route6_call_use_source_adapter.md`
- `ideas/closed/206_route7_comparison_provenance_diagnostic_oracle.md`

Those follow-ups should now be treated as adapter/diagnostic evidence, not as
new schema evidence. C2 must decide what they actually changed for contraction
readiness. A selected adapter may make one cache/API surface thinner, but it
may also prove that prepared fallback remains required and that no contraction
is safe yet.

## Shared Artifact Contract

This phase must read:

- `docs/bir_prealloc_fusion/phase_a2_residual_semantic_owner_audit.md`
- `docs/bir_prealloc_fusion/phase_b2_selected_route_extension_schema_readiness.md`
- `ideas/closed/201_phase_b2_selected_route_extension_schema_readiness_audit.md`
- `ideas/closed/202_route3_memory_source_identity_adapter.md`
- `ideas/closed/203_route4_publication_identity_adapter.md`
- `ideas/closed/204_route5_edge_join_source_adapter.md`
- `ideas/closed/205_route6_call_use_source_adapter.md`
- `ideas/closed/206_route7_comparison_provenance_diagnostic_oracle.md`
- `docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md`
- `docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md`
- `docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md`
- current accepted `test_baseline.log`

This phase must write its durable analysis payload to:

- `docs/bir_prealloc_fusion/phase_c2_selected_adapter_cache_contraction_readiness.md`

## Direction

C2 decides surface fate, not consumer migration order.

For each prepared/prealloc surface touched by the Phase B2 adapters, classify
it as one of:

- ready for a bounded micro-contraction idea;
- private/pass-context ready only after a named consumer migration;
- retained public fallback/oracle;
- retained target/prepared policy;
- diagnostic/oracle replacement prerequisite;
- blocked/unknown pending more evidence.

## In Scope

- Prepared lookup/cache/API surfaces touched by Route 3, Route 4, Route 5,
  Route 6, and Route 7 adapter/diagnostic closures.
- `PreparedFunctionLookups` groups affected by those selected adapters.
- Prepared printer/debug/oracle surfaces affected by Route 7 diagnostic work.
- Whether any adapter eliminated a public consumer boundary or merely added a
  route-first read with prepared fallback.
- Whether accepted baseline and backend proof are sufficient for contraction
  evidence or only for adapter correctness.
- Follow-up ideas for one cache/API surface at a time when contraction is
  actually ready.

## Out Of Scope

- Directly deleting prepared APIs in this analysis phase.
- Planning consumer migration ladders; that belongs to D2.
- Contracting target-policy surfaces.
- Treating adapter success as schema, consumer exhaustion, or diagnostic
  replacement.
- Combining diagnostics, cache contraction, and consumer migration in one broad
  implementation idea.
- Opening draft 155.

## Required Analysis

For each adapter or diagnostic closure 202-206, record:

- selected reader or diagnostic row;
- prepared surface touched;
- whether any public prepared consumer was removed;
- prepared fallback/oracle retained;
- target/prepared policy retained;
- tests/proof scope;
- contraction readiness result.

Then classify affected surfaces across:

- Route 3 memory/source helpers and `memory_accesses`;
- Route 4 publication helpers and `edge_publications`;
- Route 5 edge/join-source helpers and move/edge publication surfaces;
- Route 6 call-use helpers and `call_plans`;
- Route 7 comparison/provenance diagnostics and comparison helper surfaces;
- aggregate `PreparedFunctionLookups` coupling;
- prepared diagnostics/oracles and string-authority surfaces.

## Expected Output

The closure note must contain:

- a link to
  `docs/bir_prealloc_fusion/phase_c2_selected_adapter_cache_contraction_readiness.md`;
- a surface-by-surface contraction readiness table;
- any accepted micro-contraction follow-up ideas;
- surfaces explicitly retained as public fallback/oracle or target policy;
- diagnostics/oracle replacement prerequisites;
- explicit blockers to `PreparedFunctionLookups` and `PreparedBirModule`
  retirement;
- a recommendation for how D2 should be rewritten or opened based on the C2
  findings.

## Reviewer Reject Signals

- Claiming cache/API contraction because a selected adapter was green.
- Deleting or privatizing a prepared helper while any production,
  printer/debug, target-wrapper, or oracle consumer remains.
- Hiding prepared fallbacks needed for absent, mismatched, ambiguous,
  duplicate/conflict, invalid-reference, or policy-sensitive facts.
- Treating full-suite greenness as contraction readiness.
- Moving target-policy, printer/oracle authority, ABI/layout, move scheduling,
  branch policy, or final emission records into BIR.
- Opening D2 as a migration plan that ignores C2 retained-surface decisions.
- Claiming broad `PreparedFunctionLookups` or `PreparedBirModule` retirement
  from one selected cache/API surface.
