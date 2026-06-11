Status: Active
Source Idea Path: ideas/open/201_phase_b2_selected_route_extension_schema_readiness_audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish Evidence Matrix

# Current Packet

## Just Finished

Completed plan Step 1: Establish Evidence Matrix. Created
`docs/bir_prealloc_fusion/phase_b2_selected_route_extension_schema_readiness.md`
as the Phase B2 schema-readiness evidence matrix from the required source
documents. The matrix names every A2 accepted Route 1-8 candidate and records
semantic fact, accepted boundary, excluded target/prepared policy, and retained
fallback/oracle surface for each route.

## Suggested Next

Execute Step 2 from `plan.md`: classify whether existing route records/indexes
are sufficient for each Route 1-8 candidate, or whether the row belongs to a
small schema/index extension, compatibility adapter, diagnostic/oracle
replacement, retained-policy cleanup, or no action.

## Watchouts

- This is analysis-only; do not edit implementation files.
- Preserve Phase A2 excluded target/prepared policy for every candidate.
- Do not weaken prepared oracle coverage, expected strings, baseline
  requirements, or string-authority rules.
- Route 8 is citation-oriented here: its public prepared return-chain helper
  surface is absent after idea 180, but target return policy still remains
  outside Route 8.
- Idea 190 remains mandatory for Route 3: semantic memory/source identity does
  not retire prepared target-addressing fallback.

## Proof

Docs-only inspection and targeted searches. Delegated searches run:

- `rg -n "Route [1-8]|selected|accepted|excluded|fallback|oracle|string-authority" docs/bir_prealloc_fusion/phase_a2_residual_semantic_owner_audit.md docs/bir_prealloc_fusion/phase_a_normalization_candidates.md docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md docs/bir_prealloc_fusion/return_chain_import_and_naming.md docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md ideas/closed/190_full_suite_baseline_99_percent_regression_attribution.md ideas/closed/199_full_suite_baseline_string_authority_timeout_attribution.md`
- `rg -n "Route [1-8]|Phase B2|schema-readiness|prepared|oracle|string-authority" docs/bir_prealloc_fusion/phase_b2_selected_route_extension_schema_readiness.md todo.md`

No build or tests were run because this packet was docs-only. `test_after.log`
was not touched because the packet explicitly listed it under Do Not Touch.
