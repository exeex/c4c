# 226 Phase E3 prepared diagnostic/oracle replacement readiness

## Goal

Analyze which prepared diagnostic, printer/debug, route-debug, helper-oracle,
or expected-string row can be replaced or augmented with BIR/route-native
evidence after E1 and E2 proved selected semantic identity and private
pass-context boundaries.

This is Phase E3 of the BIR/prealloc thinning program. This activation is
analysis-only. It must produce follow-up implementation ideas for one
diagnostic/oracle row or one tightly scoped row family at a time; it must not
directly rewrite diagnostics, refresh baselines, or delete prepared oracles.

## Why This Exists

Phase E0 established that prepared diagnostics and string authority are not
optional cleanup. They are the comparison surface that proves route/prepared
agreement while BIR takes semantic ownership and prepared retains policy.

Phase E1 and E2 then closed selected implementation slices:

- `ideas/closed/219_phase_e1_control_flow_identity_helper_contraction.md`
- `ideas/closed/220_phase_e1_route_identity_helper_contraction.md`
- `ideas/closed/224_phase_e2_control_flow_branch_target_helper_private_pass_context.md`
- `ideas/closed/225_phase_e2_fused_compare_operand_producer_helper_private_pass_context.md`

Those closures prove two narrow families now have route/BIR semantic evidence
and private pass-context boundaries:

- branch-target identity through BIR structured successor facts;
- fused-compare operand producer identity through Route 7/prepared agreement.

E3 must decide whether any diagnostic/oracle row around those proven families,
or around earlier Route 3 through Route 7 row evidence, is ready for a
route-native diagnostic follow-up. It must preserve prepared fallback and
byte-stable output unless a later implementation idea explicitly proves a
semantic output change.

## Required Inputs

This phase must read:

- `ideas/closed/217_phase_e0_prepared_bir_merge_readiness_and_ownership_boundary_audit.md`
- `docs/bir_prealloc_fusion/phase_e0_prepared_bir_merge_readiness.md`
- `ideas/closed/218_phase_e1_semantic_duplicate_candidate_triage.md`
- `docs/bir_prealloc_fusion/phase_e1_semantic_duplicate_candidate_triage.md`
- `ideas/closed/223_phase_e2_prepared_lookup_api_private_pass_context_readiness.md`
- `docs/bir_prealloc_fusion/phase_e2_prepared_lookup_api_private_pass_context_readiness.md`
- `ideas/closed/219_phase_e1_control_flow_identity_helper_contraction.md`
- `ideas/closed/220_phase_e1_route_identity_helper_contraction.md`
- `ideas/closed/224_phase_e2_control_flow_branch_target_helper_private_pass_context.md`
- `ideas/closed/225_phase_e2_fused_compare_operand_producer_helper_private_pass_context.md`
- Route 3 through Route 7 row/diagnostic closures `208`, `210`, `212`, `214`,
  and `216`
- `docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md`
- `docs/bir_prealloc_fusion/phase_d2_retained_surface_consumer_switch_analysis.md`
- current accepted baseline state and hook review state.

This phase must write its durable analysis payload to:

- `docs/bir_prealloc_fusion/phase_e3_prepared_diagnostic_oracle_replacement_readiness.md`

## Direction

E3 decides diagnostic/oracle replacement readiness. It does not implement row
replacement itself.

For each candidate row or row family, classify it as one of:

- ready to draft one diagnostic/oracle implementation idea;
- proof harness only for an E1/E2 semantic helper and not independently
  replaceable;
- retained prepared oracle/fallback authority;
- retained target/prepared policy or emitted-output authority;
- cross-target wrapper prerequisite owned by E4;
- blocked by expected-string, baseline, or unsupported-path authority;
- no-action.

Accepted E3 follow-up ideas must name exactly one diagnostic/oracle/string row
or one tightly scoped row family, the BIR/route facts that explain the positive
row, the prepared fallback/oracle authority that remains for non-agreement
paths, and the proof required before output or oracle ownership can change.

## Candidate Surfaces

Start from:

- helper-oracle rows for
  `find_prepared_control_flow_branch_target_labels(...)`;
- helper-oracle, branch-control, and machine-printer rows around
  `find_prepared_fused_compare_operand_producer_facts(...)`;
- Route 3 memory/source helper-oracle or prepared addressing printer rows;
- Route 4 block-entry publication printer/debug rows;
- Route 5 edge/join helper-oracle or prepared printer rows;
- Route 6 x86 scalar source route-debug rows;
- Route 7 fused-compare or materialized-condition helper-oracle rows;
- any E0/E1 row-scoped diagnostic/oracle candidate that already has a proven
  semantic owner and fallback boundary.

## In Scope

- One diagnostic/oracle/string row or tightly scoped row family at a time.
- Positive route/BIR-native explanation of the row.
- Prepared fallback for absent, invalid, duplicate/conflict, mismatch,
  unsupported, and non-agreement paths.
- Byte-stable printer/debug, route-debug, helper-oracle, wrapper, and
  expected-string behavior unless a later implementation idea separately
  justifies a semantic output change.
- Follow-up implementation ideas for safe one-row replacement or augmentation.

## Out Of Scope

- Direct implementation.
- Broad prepared printer, CLI dump, route-debug, or helper-oracle replacement.
- Baseline refreshes, helper renames, unsupported downgrades, timeout masking,
  or expected-string rewrites as proof.
- Public prepared API contraction; that belongs to E2.
- Cross-target wrapper migration; that belongs to E4.
- Aggregate `PreparedFunctionLookups`, `PreparedBirModule`, draft 155, or E5
  retirement work.
- Moving target ABI/layout/register/stack/address/move/call/branch/emission
  policy into BIR or route authority.

## Expected Output

The closure note must contain:

- a link to
  `docs/bir_prealloc_fusion/phase_e3_prepared_diagnostic_oracle_replacement_readiness.md`;
- a candidate-by-candidate diagnostic/oracle readiness table;
- any accepted follow-up implementation ideas, each scoped to exactly one row
  or tightly scoped row family;
- explicit retained-authority decisions for prepared fallback/oracle,
  target-policy, wrapper, expected-string, and baseline surfaces;
- deferrals to E1, E2, E4, Route 8, or E5 where E3 is not the correct owner;
- proof requirements for accepted implementation ideas, including positive,
  absent, invalid, duplicate/conflict, mismatch, fallback, wrapper,
  printer/debug, helper-oracle, and expected-string behavior;
- an explicit statement that draft 155 / E5 and broad diagnostic/oracle
  replacement remain unopened.

## Reviewer Reject Signals

- Using production greenness as diagnostic/oracle replacement proof.
- Weakening or deleting the oracle that proves route/prepared equivalence.
- Replacing broad printer/debug or CLI dump sections from one row's evidence.
- Counting expected-string rewrites, helper renames, unsupported downgrades,
  timeout masking, or baseline refreshes as progress.
- Moving target policy or emitted-output authority into BIR/route diagnostics.
- Opening draft 155 or claiming broad prepared retirement from E3 readiness.
