# Pointer-Value Memory-Use Freshness Authority

Status: Open
Type: Architecture contract and narrow implementation
Parent: `ideas/closed/597_pointer_address_semantic_model_research.md`
Related:
- `ideas/closed/597_pointer_address_semantic_model_research.md`
- `ideas/closed/587_prepared_value_freshness_authority_mvp.md`
- `ideas/closed/589_direct_edge_publication_move_freshness_ownership.md`
Owning Layer: shared prepared/prealloc pointer-value memory-use authority

## Goal

Define selected freshness authority for pointer-value indirect memory uses so a
named pointer value is proven fresh for the exact load/store use before
prepared range/layout proof or target memory operands are treated as semantic
permission.

This route should keep address legality and pointer-value freshness separate:
`prepared_pointer_value_memory_has_proven_authority(...)`-style range and
layout checks can support a route, but they must not prove that the pointer
value itself is current.

## Why This Exists

The pointer/address semantic model research classified pointer-value indirect
memory access as deferred. Current prepared access records preserve pointer
value name, offset, layout authority, extent, and range, and target consumers
can form scalar pointer-value load/store operands from that evidence. That is
address legality support and target-consume data, not selected freshness for
the named pointer at the consuming instruction.

Without a selected freshness owner, a consumer can drift into accepting
range-only, layout-only, or target-shape-only evidence as pointer freshness.

## Prerequisites

- `docs/pointer_address_semantic_model_research/02_semantic_authority_and_fact_classes.md`
- `docs/pointer_address_semantic_model_research/03_fail_closed_rules.md`
- Closed idea 587 selected freshness vocabulary and fail-closed lookup model.
- Closed idea 589 as evidence that source freshness must not be inferred from
  destination legality, complete homes, or local move shape.

## In Scope

- Audit pointer-value indirect memory producers and one representative
  load/store consumer.
- Define the selected freshness authority dimensions for the named pointer
  value at the exact memory use.
- Keep address legality support facts separate from pointer-value freshness.
- Add or wire one representative query only after the authority rule is
  explicit.
- Add focused proof that missing, ambiguous, stale, wrong-pointer,
  wrong-load/store-use, range-only, local-layout-only, and target-shape-only
  evidence fails closed.

## Out Of Scope

- Pointer base plus offset selected authority.
- Local-array or global static semantic GEP target consumption.
- Broad target migration across RV64, AArch64, and x86.
- Global symbol memory-access authority, loaded-value freshness, or
  store-source freshness except where needed to state this boundary.
- Expectation rewrites, unsupported-marker edits, allowlist changes, runtime
  behavior changes, harness changes, or diagnostic-only changes as proof.

## Acceptance Criteria

- The packet names the audited pointer-value memory producers and the
  representative consumer.
- The selected authority rule names the pointer value, load/store use,
  program point, proof, and required support facts.
- Range/layout proof remains support evidence and does not authorize pointer
  value freshness by itself.
- Missing, ambiguous, stale, wrong-pointer, wrong-use, range-only,
  local-layout-only, and target-shape-only evidence fails closed.
- Focused proof varies pointer value name, instruction use, offset/range,
  provenance base, layout authority, and target operand shape.

## Reviewer Reject Signals

- Reject accepting a pointer-value memory use because range/layout proof,
  object extent, offset encodability, or target memory operand shape exists
  while selected pointer freshness is absent.
- Reject testcase-shaped matching for one named load/store, source file,
  target operand form, or diagnostic.
- Reject broad mixed ownership that combines pointer-value memory freshness
  with pointer arithmetic, semantic GEP target consumption, and MIR view
  design.
- Reject treating diagnostics, prepared dumps, or final assembly as authority.
- Reject expectation downgrades, unsupported-marker edits, allowlist edits, or
  runtime-output changes as semantic progress.

## Closure Note Requirements

The closure note must answer:

1. Which pointer-value memory producers and consumers were audited?
2. What selected authority dimensions authorize pointer-value freshness for a
   memory use?
3. Which support facts are required but insufficient alone?
4. Which representative consumer, if any, was migrated?
5. Which stale, wrong-pointer, wrong-use, range-only, layout-only, and
   target-shape-only cases fail closed?
6. Which adjacent loaded-value, store-source, pointer arithmetic, or semantic
   GEP questions remain separate follow-ups?
