# Pointer Base Plus Offset Selected Authority

Status: Open
Type: Architecture contract and narrow implementation
Parent: `ideas/closed/597_pointer_address_semantic_model_research.md`
Related:
- `ideas/closed/597_pointer_address_semantic_model_research.md`
- `ideas/closed/587_prepared_value_freshness_authority_mvp.md`
- `ideas/closed/588_shared_prealloc_move_operand_source_freshness_inventory.md`
Owning Layer: shared prepared/prealloc pointer-arithmetic authority

## Goal

Define and publish selected authority for
`PreparedValueHomeKind::PointerBasePlusOffset` uses so pointer base freshness,
result pointer identity, byte delta, use kind, and program point are checked
before any consumer treats a computed pointer home as semantically current.

This route should migrate at most one representative consumer after the
authority rule is explicit. The migration must prove support facts and target
offset encodability do not substitute for base freshness.

## Why This Exists

The pointer/address semantic model research classified pointer base plus offset
homes as support and target evidence until a selected pointer-arithmetic
authority exists. Current facts preserve base value identity, optional symbol
identity, and byte delta, but surveyed consumers either reject the shape or
inspect it as a materialization or target-lowering form.

Without a selected authority, a later consumer could accidentally treat
computed-address shape, stack/register placement, or encodable offsets as proof
that the derived pointer is fresh for a semantic use.

## Prerequisites

- `docs/pointer_address_semantic_model_research/02_semantic_authority_and_fact_classes.md`
- `docs/pointer_address_semantic_model_research/03_fail_closed_rules.md`
- Closed idea 587 selected freshness vocabulary and fail-closed lookup model.

## In Scope

- Audit live consumers of `PreparedValueHomeKind::PointerBasePlusOffset` and
  `PreparedPointerBasePlusOffsetFact`.
- Define the selected authority dimensions for pointer arithmetic:
  base pointer freshness, result pointer identity, byte delta, use kind, and
  program point.
- Decide whether an existing freshness use kind is sufficient or whether a
  distinct pointer-arithmetic use/source kind is required.
- Publish or query the authority for one representative route only after the
  rule is explicit.
- Add focused proof that missing, ambiguous, stale, wrong-base, wrong-result,
  wrong-delta, wrong-use, range-only, and target-shape-only evidence fails
  closed.

## Out Of Scope

- Broad RV64, AArch64, or x86 target migration.
- Pointer-value indirect memory-use freshness.
- Semantic GEP target consumption.
- Relocation/materialization semantics.
- Expectation rewrites, unsupported-marker edits, allowlist changes, runtime
  behavior changes, harness changes, or diagnostic-only changes as proof.

## Acceptance Criteria

- The packet names the audited pointer-base-plus-offset consumer set.
- The selected authority rule states the required value identity, use kind,
  delta, proof, and program point.
- One representative consumer, if migrated, requires the selected authority
  before accepting the computed pointer as semantically current.
- Missing, ambiguous, stale, wrong-base, wrong-result, wrong-delta, wrong-use,
  range-only, and target-shape-only evidence fails closed.
- Focused proof shows pointer-base-plus-offset facts, stack/register homes, and
  target-encodable offsets remain insufficient without selected authority.

## Reviewer Reject Signals

- Reject accepting a computed pointer use because the home shape, byte delta,
  stack/register placement, or target offset is encodable while selected base
  freshness is absent.
- Reject testcase-shaped matching for one named source file, one target, one
  offset value, or one diagnostic string.
- Reject overloading unrelated branch, edge-publication, move, or alias
  freshness kinds when pointer arithmetic needs its own ownership boundary.
- Reject broad mixed ownership that combines pointer arithmetic, pointer-value
  memory freshness, semantic GEP target consumption, and MIR view design.
- Reject expectation downgrades, unsupported-marker edits, allowlist edits,
  runtime-output changes, or diagnostic-only changes as semantic progress.

## Closure Note Requirements

The closure note must answer:

1. Which pointer-base-plus-offset consumers were audited?
2. What selected authority dimensions authorize the computed pointer use?
3. Which freshness use/source/proof kinds were reused or added?
4. Which representative consumer, if any, was migrated?
5. Which stale, wrong-value, wrong-delta, wrong-use, range-only, and
   target-shape-only cases fail closed?
6. Which adjacent pointer/address families remain separate follow-ups?
