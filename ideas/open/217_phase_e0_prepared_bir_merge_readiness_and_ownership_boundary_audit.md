# 217 Phase E0 Prepared/BIR merge readiness and ownership boundary audit

## Goal

Build the ownership boundary map for a realistic Prepared/BIR merge strategy:
BIR should own target-neutral semantics, prepared should own only target policy,
prepared APIs should shrink toward private pass context, and duplicate
semantic helpers should become the main code-deletion target.

This is Phase E0 of the BIR/prealloc thinning program. It is analysis-only. It
must decide what can move toward BIR-owned route facts, indexes, identities, or
annotations; what must remain target/prepared policy; what can be downgraded
from public durable API to private codegen pass context; and which duplicate
semantic helper families are realistic deletion or contraction candidates.

## Why This Exists

Phase D and Phase D2 proved many selected route-first readers and diagnostic
rows, but their closure notes explicitly reject broad prepared API deletion,
aggregate `PreparedFunctionLookups` retirement, and broad `PreparedBirModule`
retirement. The next step is therefore not draft 155. The next step is a
readiness synthesis that turns those closure notes into an ownership boundary
map aimed at reducing code by removing duplicated semantic authority, not by
renaming `PreparedBirModule` or hiding a second IR behind a facade.

## Target End State

E0 should frame the later E1-E5 work around these goals:

- BIR owns semantics: route facts, semantic indexes, source/destination
  identities, producer/publication/call-use/comparison provenance, and
  target-neutral annotations should gradually become BIR-owned.
- Prepared owns only policy: ABI placement, layout, physical register and stack
  decisions, address materialization legality, move scheduling, branch
  spelling, emission records, and target-wrapper behavior stay
  prepared/target-owned unless a later phase names a replacement owner.
- Prepared APIs become private pass context: prepared lookup and module APIs
  should stop being durable public IR surfaces where consumers can instead read
  BIR semantics plus retained prepared policy/fallback through a private
  codegen context.
- Delete duplicate semantic helpers: the expected code-size reduction should
  come from retiring helpers that duplicate BIR semantic facts, not from
  moving target policy into BIR or replacing one aggregate with another.

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
`PreparedFunctionLookups` lookup group against the target end state above.
Use these classifications:

- BIR-owned semantic fact/index/identity candidate;
- BIR annotation candidate;
- duplicate semantic helper deletion/contraction candidate;
- private/transient pass context;
- retained target/prepared policy;
- retained public fallback/oracle;
- diagnostic/string authority;
- cross-target compatibility boundary;
- blocked/unknown pending more evidence.

For each candidate that looks code-size-positive, record the exact helper or
API family that could eventually be deleted, the BIR fact that would replace
its semantic authority, and the prepared fallback/policy/oracle surface that
must remain until later proof exists.

## In Scope

- `PreparedBirModule` field ownership.
- `PreparedFunctionLookups` group ownership.
- Selected Route 1-8 semantic facts versus prepared policy/fallback.
- Public prepared API surfaces that may be candidates for future
  private/pass-context contraction.
- Duplicate prepared semantic helpers that overlap existing BIR route facts,
  indexes, or identities.
- Candidate code-deletion paths where the replacement is semantic ownership
  movement rather than rename-only facade work.
- Diagnostic, printer/debug, route-debug, helper-oracle, and expected-string
  authority.
- AArch64, x86, and riscv consumer boundaries as readiness evidence.
- Follow-up draft recommendations for E1 through E5.

## Out Of Scope

- Direct implementation.
- Direct deletion or privatization of prepared APIs.
- Opening draft 155.
- Renaming `PreparedBirModule` into BIR without reducing ownership.
- Creating a BIR-owned clone of prepared target-policy data.
- Moving target ABI/layout/register/stack/emission policy into target-neutral
  BIR.
- Claiming code-size reduction from facade renames, wrapper reshuffles, or
  container moves that leave duplicate semantic helpers in place.
- Weakening tests, diagnostics, baselines, helper-oracle names, supported-path
  status, or expected strings.

## Expected Output

The closure note must contain:

- a link to `docs/bir_prealloc_fusion/phase_e0_prepared_bir_merge_readiness.md`;
- a `PreparedBirModule` field-by-field ownership map;
- a `PreparedFunctionLookups` group-by-group ownership map;
- a target-end-state table organized by:
  BIR owns semantics, prepared owns policy, prepared APIs become private pass
  context, and duplicate semantic helpers are deletion candidates;
- a list of semantic duplicate surfaces that are candidates for E1;
- a list of public prepared APIs that are candidates for E2;
- a list of helper/API families where code-size reduction is realistic, and
  the proof required before deletion;
- a list of surfaces where code-size reduction is not realistic because the
  surface is target policy, diagnostics/oracle, cross-target compatibility, or
  retained fallback;
- diagnostic/oracle replacement prerequisites for E3;
- cross-target wrapper blockers for E4;
- exact criteria for when draft 155 / E5 can be rewritten or opened;
- any follow-up implementation or analysis ideas needed before E1-E5.

## Reviewer Reject Signals

- Treating selected route-first readers as proof of broad prepared retirement.
- Classifying target policy as BIR-owned only to make deletion look easier.
- Treating a container merge, facade rename, or wrapper move as code reduction.
- Treating prepared printer/debug/oracle surfaces as optional.
- Recommending draft 155 before field ownership, fallback/oracle, diagnostic,
  string, and public-consumer blockers are mapped.
- Failing to identify duplicate semantic helper families as the primary
  deletion target.
- Combining implementation with this analysis.
