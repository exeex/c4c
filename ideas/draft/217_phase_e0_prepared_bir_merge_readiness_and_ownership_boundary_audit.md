# 217 Phase E0 Prepared/BIR merge readiness and ownership boundary audit

## Goal

Build the field-by-field and lookup-group-by-lookup-group ownership map needed
before any true Phase E `PreparedBirModule` retirement or demotion can start.

This is Phase E0 of the BIR/prealloc thinning program. It is analysis-only. It
must decide what can become BIR-owned semantic data, what must remain
target/prepared policy, what is only transient pass context, and what remains
public fallback/oracle or diagnostic/string authority.

## Why This Exists

Phase D and Phase D2 proved many selected route-first readers and diagnostic
rows, but their closure notes explicitly reject broad prepared API deletion,
aggregate `PreparedFunctionLookups` retirement, and broad `PreparedBirModule`
retirement. The next step is therefore not draft 155. The next step is a
readiness synthesis that turns those closure notes into an ownership boundary
map.

## Required Inputs

This phase must read:

- `ideas/draft/155_phase_e_prepared_bir_module_retirement_plan.md`
- `ideas/closed/181_phase_d_mir_consumer_bir_view_switch_plan.md`
- `ideas/closed/191_phase_d_followup_closure_pre_phase_e_readiness_audit.md`
- `ideas/closed/192_residual_route_view_consumer_migration_map.md`
- `ideas/closed/203_phase_d2_retained_surface_consumer_switch_analysis.md`
- `ideas/closed/207_route3_memory_source_semantic_reader.md`
- `ideas/closed/208_route3_memory_source_oracle_printer_row.md`
- `ideas/closed/209_route4_publication_source_semantic_reader.md`
- `ideas/closed/210_route4_block_entry_publication_printer_debug_row.md`
- `ideas/closed/211_route5_current_block_join_source_semantic_reader.md`
- `ideas/closed/212_route5_edge_join_oracle_printer_row.md`
- `ideas/closed/213_route6_call_source_consumer.md`
- `ideas/closed/214_route6_x86_scalar_source_route_debug_row.md`
- `ideas/closed/215_route7_comparison_provenance_consumer.md`
- `ideas/closed/216_route7_comparison_oracle_row.md`
- `docs/bir_prealloc_fusion/phase_d_followup_pre_phase_e_readiness.md`
- `docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md`
- `docs/bir_prealloc_fusion/phase_d2_retained_surface_consumer_switch_analysis.md`
- `docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md`
- `src/backend/prealloc/module.hpp`
- `src/backend/prealloc/prepared_lookups.hpp`

This phase must write its durable analysis payload to:

- `docs/bir_prealloc_fusion/phase_e0_prepared_bir_merge_readiness.md`

## Direction

Classify every `PreparedBirModule` field and every
`PreparedFunctionLookups` lookup group as one of:

- BIR semantic annotation candidate;
- BIR route/index ownership candidate;
- private/transient pass context;
- retained target/prepared policy;
- retained public fallback/oracle;
- diagnostic/string authority;
- cross-target compatibility boundary;
- blocked/unknown pending more evidence.

## In Scope

- `PreparedBirModule` field ownership.
- `PreparedFunctionLookups` group ownership.
- Selected Route 1-8 semantic facts versus prepared policy/fallback.
- Public prepared API surfaces that may be candidates for future
  private/pass-context contraction.
- Diagnostic, printer/debug, route-debug, helper-oracle, and expected-string
  authority.
- AArch64, x86, and riscv consumer boundaries as readiness evidence.
- Follow-up draft recommendations for E1 through E5.

## Out Of Scope

- Direct implementation.
- Direct deletion or privatization of prepared APIs.
- Opening draft 155.
- Renaming `PreparedBirModule` into BIR without reducing ownership.
- Moving target ABI/layout/register/stack/emission policy into target-neutral
  BIR.
- Weakening tests, diagnostics, baselines, helper-oracle names, supported-path
  status, or expected strings.

## Expected Output

The closure note must contain:

- a link to `docs/bir_prealloc_fusion/phase_e0_prepared_bir_merge_readiness.md`;
- a `PreparedBirModule` field-by-field ownership map;
- a `PreparedFunctionLookups` group-by-group ownership map;
- a list of semantic duplicate surfaces that are candidates for E1;
- a list of public prepared APIs that are candidates for E2;
- diagnostic/oracle replacement prerequisites for E3;
- cross-target wrapper blockers for E4;
- exact criteria for when draft 155 / E5 can be rewritten or opened;
- any follow-up implementation or analysis ideas needed before E1-E5.

## Reviewer Reject Signals

- Treating selected route-first readers as proof of broad prepared retirement.
- Classifying target policy as BIR-owned only to make deletion look easier.
- Treating prepared printer/debug/oracle surfaces as optional.
- Recommending draft 155 before field ownership, fallback/oracle, diagnostic,
  string, and public-consumer blockers are mapped.
- Combining implementation with this analysis.
