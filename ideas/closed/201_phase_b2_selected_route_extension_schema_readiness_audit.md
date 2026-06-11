# 201 Phase B2 selected route extension schema readiness audit

## Goal

Audit whether the narrow Route 1-8 extension/import candidates accepted by
Phase A2 need additional BIR annotation/index schema work, compatibility
adapters, or only selected-consumer follow-up ideas.

This is Phase B2 of the BIR/prealloc thinning program. It is analysis-only and
must not implement schemas, migrate consumers, contract prepared APIs, or open
the Phase E `PreparedBirModule` retirement plan.

## Why This Exists

Phase A2 closed with
`docs/bir_prealloc_fusion/phase_a2_residual_semantic_owner_audit.md`. It did
not accept any brand-new BIR route. Instead, it accepted only narrow existing
Route 1-8 extension/import candidates and rejected broad residual prepared
surfaces as target/prepared policy, transient pass context, diagnostic/oracle
compatibility, wrapper compatibility, or baseline/string-authority guardrails.

Therefore the original residual-schema draft must not be opened mechanically
as a new-route schema audit. Phase B2 should now inspect the accepted A2
extension candidates and decide, one by one, whether they require:

- new annotation/index schema records;
- a small extension to an existing Route 1-8 schema;
- only a compatibility adapter or diagnostic/oracle plan;
- no schema work because the surface remains target/prepared policy.

## Shared Artifact Contract

This phase must read:

- `docs/bir_prealloc_fusion/phase_a2_residual_semantic_owner_audit.md`
- `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`
- `docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md`
- `docs/bir_prealloc_fusion/return_chain_import_and_naming.md`
- `docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md`
- `docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md`
- `ideas/closed/190_full_suite_baseline_99_percent_regression_attribution.md`
- `ideas/closed/199_full_suite_baseline_string_authority_timeout_attribution.md`

This phase must write its durable analysis payload to:

- `docs/bir_prealloc_fusion/phase_b2_selected_route_extension_schema_readiness.md`

## Direction

Design or extend schemas only for facts that Phase A2 already classified as
target-neutral semantic identity inside Routes 1-8. Do not create broad new
routes unless the source evidence contradicts the Phase A2 closure, and do not
move target/layout/codegen policy into BIR records.

## Accepted A2 Candidate Set

Phase B2 should inspect only these A2-accepted candidates:

- Route 1 selected producer/constant consumer or diagnostic row.
- Route 2 selected select-chain/direct-global consumer or diagnostic row.
- Route 3 selected memory/source identity consumer, with idea 190 fallback
  discipline.
- Route 4 selected publication identity or wrapper adapter.
- Route 5 selected edge/join-source identity or wrapper adapter.
- Route 6 selected call-use source adapter.
- Route 7 selected comparison provenance reader.
- Route 8 return-chain citation/import or diagnostic row.

Each candidate must preserve the excluded target/prepared policy listed in the
Phase A2 artifact.

## In Scope

- Determine whether each accepted Route 1-8 extension candidate can use
  existing route records/indexes as-is.
- Identify any small schema/index extension required for a selected candidate.
- Define bridge/oracle/fallback requirements against retained prepared
  surfaces.
- Define negative, ambiguous, mismatch, invalid-reference, duplicate/conflict,
  policy-sensitive, and string-authority cases required before implementation.
- Produce follow-up ideas only for candidates with enough evidence and bounded
  proof routes.
- Retire or defer candidates that are really diagnostic/oracle replacement or
  retained-policy cleanup rather than schema work.

## Out Of Scope

- Implementing schema records, indexes, adapters, or diagnostics.
- Migrating MIR/codegen consumers.
- Deleting, privatizing, or renaming prepared helpers.
- Reclassifying Phase A2 rejected target-policy/pass-context/diagnostic
  surfaces as schema candidates without new evidence.
- Opening or executing draft 155.
- Creating a BIR-owned clone of `PreparedFunctionLookups` or
  `PreparedBirModule`.

## Required Analysis

For each accepted A2 candidate, record:

- route number and semantic fact;
- whether existing schema/index support is sufficient;
- any missing record field, id, owner scope, index, validation status, or
  fail-closed state;
- retained prepared fallback/oracle surface;
- excluded target/prepared policy;
- proof recommendations and reject signals;
- whether the right follow-up is schema implementation, adapter planning,
  diagnostic/oracle replacement, retained-policy cleanup, or no action.

## Expected Output

The closure note must contain:

- a link to
  `docs/bir_prealloc_fusion/phase_b2_selected_route_extension_schema_readiness.md`;
- a per-route Route 1-8 schema-readiness table;
- accepted schema or index extension follow-up ideas, if any;
- deferred non-schema candidates and where they should go instead;
- rejected surfaces and why they stay outside BIR;
- bridge/oracle/fallback and proof-route recommendations.

## Closure Note

Closed by the Phase B2 schema-readiness audit.

Durable closure artifact:
`docs/bir_prealloc_fusion/phase_b2_selected_route_extension_schema_readiness.md`

Closure summary:

- The artifact contains the per-route Route 1-8 schema-readiness table and
  records no missing required sources.
- No schema/index extension follow-up was accepted in this phase.
- Routes 3-6 were opened as bounded compatibility-adapter follow-up ideas, and
  Route 7 was opened as a bounded diagnostic/oracle follow-up idea.
- Routes 1-2 are deferred until a concrete selected consumer or visible
  diagnostic row is named.
- Route 8 remains no-action for schema/index extension unless a future visible
  diagnostic row is selected.
- Retained prepared-policy cleanup and broad `PreparedFunctionLookups`,
  `PreparedBirModule`, wrapper, printer, target-policy, ABI/layout,
  move-scheduling, branch-policy, or final-emission contraction are rejected
  for this phase and remain outside BIR schema scope.
- Bridge/oracle/fallback and proof-route recommendations are preserved in the
  retained cross-cutting surfaces, Step 2 retained fallback/oracle notes, and
  Step 3 proof/reject rows.

Close gate:

- Matching before/after regression logs cover
  `ctest --test-dir build -j --output-on-failure -R '^ccc_review_'`.
- Both logs show 9/9 tests passed after rebuild.
- The maintenance-mode regression guard passed:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`.

## Reviewer Reject Signals

- Opening this as a brand-new residual route/schema program despite Phase A2
  accepting no brand-new routes.
- Copying prepared wrapper structs or aggregate fields into BIR records.
- Encoding target policy, printer formatting, pass context, ABI/layout,
  storage, move scheduling, branch policy, helper protocols, or final emission
  order as BIR schema.
- Treating lack of schema support as permission for consumers to rescan BIR,
  name-match, or use testcase-shaped shortcuts.
- Weakening prepared oracle coverage, expected strings, baseline requirements,
  or string-authority rules.
- Claiming Phase B2 analysis retires prepared APIs, diagnostics, or
  `PreparedBirModule`.
