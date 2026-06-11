# 200 Phase A2 residual prepared surface semantic-owner audit

## Goal

Audit prepared/prealloc surfaces that are not adequately covered by the
existing Route 1-7 semantic families or by the imported Route 8 return-chain
line, then classify their durable owner before any further schema, contraction,
consumer-switch, or `PreparedBirModule` retirement work.

This is Phase A2 of the BIR/prealloc thinning program. It is analysis-only and
must produce follow-up ideas for new semantic route candidates, retained
target-policy surfaces, pass-context/cache surfaces, diagnostic/oracle surfaces,
and no-route blockers.

## Why This Exists

The Route 1-7 program moved core target-neutral semantic facts into BIR route
records and proved selected consumer migrations. Follow-up audits 191-198 show
that many important prepared/prealloc surfaces still remain outside those
routes. Some are probably target-neutral semantic facts that need new route
ownership; others are target policy, pass-local context, compatibility
diagnostics, or retained oracles that should not move into BIR.

Idea 190 also proved that a route fact can be locally correct while broader
backend behavior regresses if prepared fallback or target policy is bypassed.
Before another migration wave, Phase A2 must classify the residual surfaces by
ownership rather than forcing them into Route 1-7.

## Shared Artifact Contract

This phase must read:

- `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`
- `docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md`
- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- `docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md`
- `docs/bir_prealloc_fusion/phase_d_followup_pre_phase_e_readiness.md`
- `docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md`
- `docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md`
- `docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md`
- `docs/bir_prealloc_fusion/cross_target_route_view_reuse_map.md`
- `docs/bir_prealloc_fusion/return_chain_import_and_naming.md`
- `ideas/closed/190_full_suite_baseline_99_percent_regression_attribution.md`
- `ideas/open/199_full_suite_baseline_string_authority_timeout_attribution.md`
  if still open, or its closure if completed
- `ideas/draft/155_phase_e_prepared_bir_module_retirement_plan.md`

This phase must write its durable analysis payload to:

- `docs/bir_prealloc_fusion/phase_a2_residual_semantic_owner_audit.md`

## Direction

Classify residual prepared/prealloc facts by true owner before designing
schemas or moving consumers.

Use these ownership categories:

- new target-neutral BIR route candidate;
- existing Route 1-8 extension candidate;
- target/prepared policy that should stay outside BIR;
- transient pass context or private construction cache;
- diagnostic/printer/oracle compatibility surface;
- blocked or unknown surface requiring deeper source inspection.

## In Scope

- `PreparedBirModule` fields and wrapper state not already mapped to Routes
  1-8.
- `PreparedFunctionLookups` groups classified as retained or mixed in idea 196.
- AArch64, x86, and riscv prepared wrapper surfaces not covered by selected
  route-view migrations.
- Wide-value carriers, runtime helpers, variadic/intrinsic/inline-asm/atomic
  helpers, dynamic-stack/frame helpers, and special target-entry surfaces.
- Prepared printer/debug/dump/oracle surfaces that may contain semantic facts
  not yet represented route-natively.
- Any baseline or string-authority findings from idea 199 that reveal an
  ownership or lifecycle rule gap.

## Out Of Scope

- Adding new BIR records or indexes.
- Migrating MIR/codegen consumers.
- Contracting or deleting prepared APIs.
- Opening or executing draft 155.
- Forcing target policy, diagnostics, or pass context into BIR merely to make
  retirement appear closer.

## Required Analysis

For each residual surface, record:

- current files and public/private API names;
- current consumers;
- whether it represents target-neutral semantic identity, target policy,
  pass-context/cache, diagnostic/oracle output, or unknown mixed state;
- whether an existing route covers it, a new route is needed, or it should be
  explicitly retained outside BIR;
- proof or source evidence for that classification;
- follow-up idea required, if any.

## Expected Output

The closure note must contain:

- a link to `docs/bir_prealloc_fusion/phase_a2_residual_semantic_owner_audit.md`;
- a residual ownership table;
- accepted new route candidates, if any;
- rejected target-policy/pass-context/diagnostic surfaces with rationale;
- blocked/unknown surfaces needing focused audit;
- follow-up ideas for Phase B2 schema work and retained-policy cleanup.

## Reviewer Reject Signals

- Treating every residual prepared surface as a BIR route candidate.
- Folding target policy, ABI/layout details, storage homes, move scheduling,
  printer formatting, or final emission records into BIR.
- Ignoring idea 190 or 199 baseline lessons when classifying residual surfaces.
- Claiming retirement readiness from classification alone.
- Omitting x86/riscv wrappers, diagnostics, or oracle consumers because they
  are not AArch64 production lowering.
- Weakening tests, baselines, or string-authority rules to make ownership look
  cleaner.
