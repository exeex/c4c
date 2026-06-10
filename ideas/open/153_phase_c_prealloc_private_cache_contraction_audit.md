# 153 Phase C prealloc private cache contraction audit

## Goal

Audit which remaining prealloc structures should become private caches or thin
pass internals once BIR owns the semantic relationships and annotation schema.

This is Phase C of the BIR/prealloc thinning program. It is analysis-only and
must produce follow-up ideas for cache privatization and API contraction.

## Shared Artifact Contract

This phase must read:

- `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`
- `docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md`
- `ideas/closed/151_phase_a_bir_normalization_candidate_audit.md`
- `ideas/closed/152_phase_b_bir_annotation_schema_candidate_audit.md`
- the closure notes for Phase B implementation follow-ups:
  - `ideas/closed/159_bir_producer_identity_annotation_schema.md`
  - `ideas/closed/160_bir_select_chain_global_dependency_annotation_schema.md`
  - `ideas/closed/161_bir_memory_access_identity_annotation_schema.md`
  - `ideas/closed/162_bir_publication_availability_annotation_schema.md`
  - `ideas/closed/163_bir_edge_join_source_annotation_schema.md`
  - `ideas/closed/164_bir_call_use_source_annotation_schema.md`
  - `ideas/closed/165_bir_comparison_condition_annotation_schema.md`
  - `ideas/closed/166_bir_annotation_lookup_index_schema.md`

This phase must write its durable analysis payload to:

- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`

The closure note should summarize that document. Phase D and Phase E must be
able to consume the artifact directly.

## Phase B Handoff Requirement

Phase B produced typed BIR annotation schemas and lookup/index prototypes for
the Phase A routes. This Phase C audit must use those closed routes as the
source of truth for cache contraction readiness.

For each Phase B route, Phase C must decide:

- which prepared lookup, index, or facade surfaces are now redundant because a
  BIR annotation/index owns the semantic answer;
- which prepared surfaces must remain public as migration oracles for Phase D;
- which prepared surfaces should become private prealloc construction caches;
- which residual consumer migrations from the Phase B closure notes block
  contraction;
- which follow-up idea should perform the bounded privatization or API
  contraction.

Known Phase B residuals to classify include:

- Route 4 block-entry MIR consumer remains on the older semantic PHI scan while
  BIR block-entry records/indexes are implemented and oracle-covered.
- Route 5 current-block join-source public helper remains on its prior
  semantic implementation while Route 5 records/indexes are implemented and
  oracle-covered.
- Route 6 has BIR call-use source records/indexes, but no broad MIR/codegen
  call consumer switch was part of the schema route.
- Route 7 future consumers must keep target branch spelling, fused-compare
  legality, condition-code selection, hazards, emitted-register state, and
  final record order outside BIR.
- Route index reference facade currently covers selected Route 4 and Route 7
  migrations, while Routes 1, 2, 3, 5, and 6 retain their existing typed
  record/index shapes.

## Direction

Prealloc should become a thin translation/planning pass, not a second IR. Its
indexes may remain for performance and one-pass construction, but those indexes
should be private implementation details unless a public semantic owner really
needs them.

## Questions

- Which lookup structs are only cache/index accelerators?
- Which public query APIs should become private helpers once equivalent BIR
  annotations exist?
- Which caches are needed only during one pass and can disappear after
  annotation fill?
- Which aggregate surfaces, especially `PreparedFunctionLookups`, remain useful
  as construction internals?
- Which current public headers should shrink after schema migration?

## Required Analysis

Inspect:

- `src/backend/prealloc/prepared_lookups.*`
- domain lookup headers and implementations introduced or cleaned by ideas
  137-150
- `src/backend/prealloc/module.hpp`
- `src/backend/prealloc/*plans*`
- `src/backend/prealloc/stack_layout/`
- AArch64 consumers that still depend on prealloc query types directly

## Expected Output

The closure note must contain:

- a link to `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`;
- a Phase B route coverage table showing each schema route's cache contraction
  status, blockers, and whether prepared remains public, private, or oracle-only;
- cache/index groups that should become private;
- cache/index groups that should remain public temporarily, with blockers;
- follow-up ideas for privatizing each concrete cache surface;
- required BIR annotation prerequisites before each privatization;
- proof-route recommendations for API contraction work.

The artifact must name the Phase A/B prerequisite section for each proposed
cache contraction route.

## Reject Signals

- Deleting caches before equivalent BIR annotations or private pass users are
  ready.
- Making consumers rebuild expensive indexes manually.
- Treating private-cache contraction as permission to weaken prepared facts.
- Moving target-local MIR lowering details into prealloc to keep caches alive.
