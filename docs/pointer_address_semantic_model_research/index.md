# Pointer/Address Semantic Model Research

Status: Step 6 complete
Source Idea: `ideas/open/597_pointer_address_semantic_model_research.md`

This directory is the research package for the pointer/address semantic model.
It separates semantic pointer/address authority from verifier/support facts,
target-consume facts, route proofs, and diagnostic-only artifacts.

## Evidence Inputs Confirmed

- Source intent: `ideas/open/597_pointer_address_semantic_model_research.md`
- Downstream boundary: `ideas/open/591_prepared_mir_view_contract_research.md`
- Selected freshness authority baseline:
  - `ideas/closed/587_prepared_value_freshness_authority_mvp.md`
  - `ideas/closed/588_shared_prealloc_move_operand_source_freshness_inventory.md`
  - `ideas/closed/589_direct_edge_publication_move_freshness_ownership.md`
  - `ideas/closed/590_branch_stack_load_freshness_contract.md`
- Narrow branch pointer stack-source evidence:
  - `ideas/closed/592_typed_aggregate_branch_stack_source_publication.md`
  - `ideas/closed/593_rv64_branch_stack_source_freshness_consumption.md`
  - `ideas/closed/594_rv64_branch_stack_source_consumption_followup_from_593.md`
  - `ideas/closed/596_pointer_rhs_branch_stack_source_policy_publication.md`
- Related docs to cite or distinguish:
  - `docs/target_abi_contract_research/index.md`
  - `docs/target_abi_contract_research/04_current_prepared_value_consumption_model.md`
  - `docs/target_abi_contract_research/05_prior_preservation_freshness_and_stale_home_risk.md`
  - `docs/prepared_fact_contracts/README.md`
  - `docs/prepared_fact_contracts/contract_taxonomy_and_fact_family_matrix.md`

No required evidence input was missing. The downstream
`docs/prepared_mir_view_contract_research/` package is not present yet because
idea 591 remains a downstream consumer, not the owner of this semantic model.

## Research Files

- [01. Pointer/Address Family Inventory](01_pointer_address_family_inventory.md)
- [02. Semantic Authority And Fact Classes](02_semantic_authority_and_fact_classes.md)
- [03. Fail-Closed Rules](03_fail_closed_rules.md)
- [04. Closed Evidence And MIR Boundary](04_closed_evidence_and_mir_boundary.md)
- [05. Follow-Up Recommendations](05_followup_recommendations.md)

## Semantic Model Summary

A pointer/address value is current for a semantic use only when the owning
semantic layer names the same value, the same use kind, the same program point
or derivation coordinate, and the proof that makes that use valid. Support,
target, route-proof, and diagnostic facts may explain or feed a route, but
they cannot promote themselves into semantic authority.

The surveyed families classify as follows:

| Family | Final classification |
| --- | --- |
| Pointer base plus offset value homes | Deferred until a selected pointer-arithmetic authority names base freshness, result identity, delta, use, and program point. |
| BIR pointer arithmetic materialization for frame addresses | Verifier/support and target-consume evidence; not freshness authority. |
| Semantic relocation and global address materialization | Target-consume evidence plus symbol-policy support; not pointer freshness authority. |
| Global symbol memory accesses | Semantic address/range authority for exact symbol-backed memory access; not loaded-value, store-source, pointer-value, or GEP freshness. |
| Local stack or frame-slot addressing | Verifier/support and target-consume evidence unless paired with separate use-specific freshness authority. |
| Pointer-value indirect memory accesses | Deferred until selected pointer-value memory-use freshness names the pointer value and exact load/store use. |
| Local-array source object and address derivation | Route proof and verifier/support; authority is the semantic GEP availability record, not raw derivation artifacts. |
| Local-array semantic GEP availability | Semantic authority for selected local-array address derivation when status is `Available`. |
| Global static semantic GEP | Semantic authority for selected global static GEP address derivation when status is `Available`. |
| Branch pointer stack-source operands | Semantic freshness authority for the exact closed RV64 fused pointer branch stack-slot `Lhs`/`Rhs` use only. |
| Target-local operand shape | Target-consume and diagnostic-only; never semantic authority. |

## Fact Classes

- Semantic authority: required to decide whether a pointer/address value is
  current for a specific use.
- Verifier/support fact: proves identity, range, layout, route coherence, or
  ordering, but cannot authorize a semantic use by itself.
- Target-consume fact: data a backend needs after semantic authority exists.
- Route proof: evidence for diagnostics, review, or debugging.
- Diagnostic-only artifact: observation such as dumps, candidate counts,
  unsupported messages, or final printed text.

Stack-home completeness, local-memory layout, relocation materialization,
target operand shape, and diagnostic dumps are not semantic authority by
themselves.

## Fail-Closed Rules

Consumers must reject semantic authority when evidence is missing, ambiguous,
stale, names the wrong value, names the wrong use, is stack-home-only,
local-layout-only, relocation-only, range-only, target-shape-only, or
diagnostic-only. The rejection should keep the route unavailable, return no
lowering, or emit an unsupported diagnostic instead of inferring from nearby
support facts.

The proof shape for later implementation work should vary authority
dimensions: value identity, use kind, program point, derivation coordinate,
source object or symbol, range authority, and fact class. A passing proof for
one named testcase or one target operand shape is not enough.

## Evidence Boundaries

Closed ideas 587 through 590 establish the selected freshness authority
baseline. They prove that use-specific selected freshness must match value,
use, source, proof, rank, source reference, and program point before a
freshness claim is accepted.

Closed ideas 592, 593, 594, and 596 prove a narrow branch pointer stack-source
subset: RV64 fused pointer branch stack-slot `Lhs` and `Rhs` emission consumes
selected shared `BranchStackLoadSource` / `BranchStackSlot` freshness for the
exact branch terminator use. That evidence does not close pointer arithmetic,
pointer-value indirect memory, semantic GEP target consumption, relocation,
local layout, stack-home completeness, or target operand-shape semantics.

## Follow-Up Outcomes

This package opens two narrow implementation ideas:

- `ideas/open/599_pointer_base_plus_offset_selected_authority.md`
- `ideas/open/600_pointer_value_memory_use_freshness_authority.md`

Both are split by first owning layer and include prerequisites, proof surface,
and reviewer reject signals in
[05. Follow-Up Recommendations](05_followup_recommendations.md).

Deferred outcomes:

- local-array and global static semantic GEP target consumption, pending idea
  591's `PreparedMirView` shape and target migration plan
- loaded-value and store-source freshness for global or pointer-value memory
- aggregate-adjacent branch, select, call, publication, and non-branch
  pointer/address consumers whose first owner or proof surface is not settled
- relocation/materialization-only and target-local operand-shape routes unless
  a later consumer audit finds a concrete support-as-authority bug

## Impact On Idea 591

Idea 591 should consume this package as a boundary contract. `PreparedMirView`
may expose prepared pointer/address facts, selected freshness authorities,
semantic GEP availability, support facts, target-consume facts, route proofs,
diagnostic facts, and unavailable/deferred statuses.

It must not synthesize a generic pointer/address validity bit from view shape,
complete stack homes, local layout/range facts, relocation/materialization
records, target operands, candidate counts, dumps, or diagnostics. It should
preserve unresolved families as unavailable or deferred until their first
owner and proof surface are settled.

## Scope Result

This research package changes documentation, lifecycle scratch state, and the
two opened source-idea handoffs only. It does not change implementation files,
tests, expectations, unsupported markers, allowlists, runtime behavior, or
harness behavior.
