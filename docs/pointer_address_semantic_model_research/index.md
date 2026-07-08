# Pointer/Address Semantic Model Research

Status: Step 1 skeleton
Source Idea: `ideas/open/597_pointer_address_semantic_model_research.md`

This directory is the research package for the pointer/address semantic model.
It will separate semantic pointer/address authority from verifier/support
facts, target-consume facts, route proofs, and diagnostic-only artifacts.

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

## Current Answer Shape

The remaining steps should keep each answer file scoped to its assigned
question. Each semantic-authority claim must cite concrete code surfaces,
closed ideas, existing docs, or command output. Unclear families should be
marked deferred rather than expanded into implementation work.
