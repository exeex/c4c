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

This phase must write its durable analysis payload to:

- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`

The closure note should summarize that document. Phase D and Phase E must be
able to consume the artifact directly.

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
