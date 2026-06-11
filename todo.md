Status: Active
Source Idea Path: ideas/open/201_phase_b2_selected_route_extension_schema_readiness_audit.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Existing Schema Sufficiency

# Current Packet

## Just Finished

Completed plan Step 2: Classify Existing Schema Sufficiency. Updated
`docs/bir_prealloc_fusion/phase_b2_selected_route_extension_schema_readiness.md`
with schema-readiness classifications for every Route 1-8 candidate. Each row
now states whether existing route records/indexes are schema sufficient,
whether future work is a small schema/index extension, compatibility adapter,
diagnostic/oracle replacement, retained-policy cleanup, or no action, and which
prepared fallback/oracle surface must remain.

## Suggested Next

Execute Step 3 from `plan.md`: extract any follow-up payloads from the Step 2
classification, keeping them narrow and route-specific.

## Watchouts

- This is analysis-only; do not edit implementation files.
- Preserve Phase A2 excluded target/prepared policy for every candidate.
- Do not weaken prepared oracle coverage, expected strings, baseline
  requirements, or string-authority rules.
- Route 8 remains no-action for schema/index extension in this phase; only a
  future visible diagnostic row would need helper-equivalence proof.
- Idea 190 remains mandatory for Route 3: semantic memory/source identity does
  not retire prepared target-addressing fallback.
- Routes 1-7 are generally schema sufficient for selected semantic facts, but
  wrappers, diagnostics, target policy, and aggregate lookup contraction remain
  separate adapter/oracle work.

## Proof

Docs-only inspection and targeted searches. Delegated searches run for Step 2:

- `rg -n "Route [1-8]|record|index|lookup|annotation|schema|adapter|oracle|fallback|route8|Route8|PreparedFunctionLookups|PreparedBirModule" docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md`
- `rg -n "Route [1-8]|record|index|lookup|annotation|schema|adapter|oracle|fallback|route8|Route8|return-chain" docs/bir_prealloc_fusion/phase_a2_residual_semantic_owner_audit.md docs/bir_prealloc_fusion/return_chain_import_and_naming.md`
- `rg -n "Route [1-8]|record|index|lookup|annotation|schema|adapter|oracle|fallback|PreparedFunctionLookups|PreparedBirModule" docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`
- `rg -n "struct Route[1-8]|using Route[1-8]|enum class Route[1-8]|route[1-8]_build|route[1-8]_find|route[1-8]_.*record" src/backend/bir/bir.hpp`
- `rg -n "Route [1-8]|schema sufficient|schema/index extension|compatibility adapter|diagnostic/oracle|retained-policy cleanup|no action|fallback|oracle" docs/bir_prealloc_fusion/phase_b2_selected_route_extension_schema_readiness.md todo.md`

No build or tests were run because this packet was docs-only. `test_after.log`
was not touched because the packet explicitly listed it under Do Not Touch.
