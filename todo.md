Status: Active
Source Idea Path: ideas/open/201_phase_b2_selected_route_extension_schema_readiness_audit.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Define Proof And Reject Coverage

# Current Packet

## Just Finished

Completed plan Step 3: Define Proof And Reject Coverage. Updated
`docs/bir_prealloc_fusion/phase_b2_selected_route_extension_schema_readiness.md`
with proof recommendations and concrete reviewer reject signals for every
Route 1-8 classification. Each row now names the minimum route/prepared
agreement, fail-closed, fallback, oracle, and string-authority proof expected
before future implementation or diagnostic work, while keeping rejected
target/prepared policy outside BIR schema scope.

## Suggested Next

Execute Step 4 from `plan.md`: open only the bounded follow-up ideas justified
by the completed audit, keeping each idea route-specific and proof/reject
coverage-driven.

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
- Step 3 did not create follow-up idea files; Step 4 should decide whether any
  are justified.

## Proof

Docs-only inspection and targeted searches. Delegated searches run for Step 3:

- `rg -n "negative|ambiguous|mismatch|invalid-reference|duplicate|conflict|policy-sensitive|string-authority|fail-closed|fallback|oracle|Route [1-8]|route8|Route8" docs/bir_prealloc_fusion/phase_a2_residual_semantic_owner_audit.md docs/bir_prealloc_fusion/phase_a_normalization_candidates.md docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md docs/bir_prealloc_fusion/return_chain_import_and_naming.md docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md ideas/closed/190_full_suite_baseline_99_percent_regression_attribution.md ideas/closed/199_full_suite_baseline_string_authority_timeout_attribution.md`
- `rg -n "Route[1-8].*(fail|invalid|duplicate|conflict|mismatch|ambiguous|unsupported)|fail_closed|fail-closed|invalid-reference|duplicate|conflict|mismatch|ambiguous|route8" src tests docs/bir_prealloc_fusion -g '!docs/bir_prealloc_fusion/phase_b2_selected_route_extension_schema_readiness.md'`
- `rg -n "negative|ambiguous|mismatch|invalid-reference|duplicate|conflict|policy-sensitive|string-authority|proof|reject|fallback|oracle|Route [1-8]" docs/bir_prealloc_fusion/phase_b2_selected_route_extension_schema_readiness.md todo.md`

No build or tests were run because this packet was docs-only. `test_after.log`
was not touched because the packet explicitly listed it under Do Not Touch.
