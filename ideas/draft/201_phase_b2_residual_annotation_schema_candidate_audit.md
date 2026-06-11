# 201 Phase B2 residual annotation schema candidate audit

## Goal

For residual surfaces accepted by Phase A2 as target-neutral semantic owner
candidates, design candidate BIR annotation/index schemas and reject surfaces
that should remain target/prepared policy, pass context, diagnostics, or
oracles.

This is Phase B2 of the BIR/prealloc thinning program. It is analysis-only and
must produce follow-up implementation ideas for accepted residual route/schema
families.

## Prerequisite

Do not activate this draft until Phase A2 has closed and produced:

- `docs/bir_prealloc_fusion/phase_a2_residual_semantic_owner_audit.md`

If Phase A2 finds no new target-neutral semantic route candidates, this draft
should be retired or rewritten rather than opened mechanically.

## Shared Artifact Contract

This phase must read:

- Phase A2 durable artifact;
- existing Phase A/B route schema artifacts;
- return-chain Route 8 import notes;
- `docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md`;
- `docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md`;
- relevant baseline recovery closure notes, including ideas 190 and 199.

This phase must write its durable analysis payload to:

- `docs/bir_prealloc_fusion/phase_b2_residual_annotation_schema_candidates.md`

## Direction

Design schemas only for residual facts that Phase A2 classifies as
target-neutral semantic identity. Keep target/layout/codegen policy out of BIR
records even when those policies are adjacent to an accepted semantic fact.

## In Scope

- Candidate record vocabulary, ids, owner scope, indexes, lookup shapes, and
  fail-closed statuses for accepted Phase A2 residual semantic families.
- Bridge/oracle rules comparing new BIR facts against retained prepared
  surfaces.
- Negative, ambiguous, mismatch, and policy-sensitive cases required before
  implementation.
- Whether accepted residual facts extend existing Routes 1-8 or require new
  route numbers.
- Follow-up implementation ideas for each accepted schema family.

## Out Of Scope

- Implementing schema records or indexes.
- Migrating consumers.
- Deleting prepared helpers or diagnostic surfaces.
- Reclassifying Phase A2 rejected target-policy surfaces as schema candidates
  without new source evidence.
- Opening draft 155.

## Expected Output

The closure note must contain:

- a link to `docs/bir_prealloc_fusion/phase_b2_residual_annotation_schema_candidates.md`;
- accepted schema candidates with route ownership;
- rejected surfaces and why they stay outside BIR;
- bridge/oracle/fallback requirements;
- proof route recommendations;
- follow-up implementation ideas for each accepted schema.

## Reviewer Reject Signals

- Copying prepared wrapper structs into BIR records.
- Encoding target policy, printer formatting, pass context, ABI/layout,
  storage, moves, or final emission order as BIR schema.
- Treating lack of a schema as permission to rescan BIR in consumers.
- Weakening prepared oracle coverage or baseline/string-authority rules.
- Claiming Phase B2 schema analysis retires prepared APIs or
  `PreparedBirModule`.
