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

## Closure Note

Closed after Phase E3 produced its durable analysis payload:
`docs/bir_prealloc_fusion/phase_e3_prepared_diagnostic_oracle_replacement_readiness.md`.

The durable document contains the candidate-by-candidate diagnostic/oracle
readiness table, retained-authority decisions, deferrals, accepted follow-up
implementation candidates, and proof requirements. E3 remains analysis-only:
it made no implementation, baseline, expected-string, unsupported-path,
prepared fallback/oracle, target-policy, wrapper, or emitted-output changes.

Accepted follow-up ideas created and left inactive:

- `ideas/open/227_phase_e3_branch_target_helper_oracle_follow_up.md`
- `ideas/open/228_phase_e3_fused_compare_operand_producer_helper_oracle_follow_up.md`
- `ideas/open/229_phase_e3_route3_memory_source_stored_value_helper_oracle_follow_up.md`
- `ideas/open/230_phase_e3_route4_block_entry_publication_printer_debug_follow_up.md`
- `ideas/open/231_phase_e3_route5_current_block_join_source_helper_oracle_follow_up.md`
- `ideas/open/232_phase_e3_route6_x86_scalar_i32_argument_source_route_debug_follow_up.md`
- `ideas/open/233_phase_e3_route7_materialized_condition_helper_oracle_follow_up.md`

Closeout classification summary:

| Candidate row or row family | Closeout decision |
| --- | --- |
| `find_prepared_control_flow_branch_target_labels(...)` helper-oracle branch-target label row | Follow-up idea `227`; prepared fallback/oracle, branch/output policy, wrappers, printer/debug, helper-oracle strings, expected strings, and aggregate APIs remain retained. |
| Adjacent branch-target printer/debug, wrapper, branch-control, edge-copy, or emitted-output rows | Retained target/prepared policy or emitted-output authority; no E3 replacement. |
| `find_prepared_fused_compare_operand_producer_facts(...)` helper-oracle operand-producer row | Follow-up idea `228`; prepared fallback/oracle, helper-oracle names/statuses, expected strings, printer/debug, wrappers, aggregate route views, and prepared aggregate ownership remain retained. |
| Fused-compare branch-control and machine-printer rows adjacent to the selected Route 7 fact | Proof harness only; retained target/prepared output authority. |
| Route 3 memory/source stored-value helper-oracle success row | Follow-up idea `229`; prepared diagnostics, target addressing policy, row text, wrappers, and expected strings remain retained for non-agreement and policy paths. |
| Route 3 prepared addressing printer row | Retained target/prepared policy or emitted-output authority. |
| Route 4 `block_entry_publication` available-register printer/debug row | Follow-up idea `230`; prepared publication mechanics, block/output policy, wrappers, row text, CLI dump scope, and expected strings remain retained. |
| Route 5 current-block join-source helper-oracle row | Follow-up idea `231`; prepared edge/join behavior, prepared-printer authority, wrappers, and expected strings remain retained. |
| Route 6 x86 scalar `i32` argument-source route-debug row | Follow-up idea `232`; `ConsumedPlans`, ABI/call wrapper behavior, prepared call printer/debug, direct-call/helper-oracle families, wrappers, and expected strings remain retained. |
| Route 7 materialized-condition helper-oracle row | Follow-up idea `233`; prepared oracle assertion strength, branch policy, branch-control output, wrappers, final assembler behavior, helper-oracle strings, and expected strings remain retained. |
| Route 7 fused-compare branch-control or machine-printer rows beyond the materialized-condition row | Proof harness only; retained target/prepared output authority. |
| Mixed Route 1/2/5/6/7 helper, printer, or oracle rows without completed row-specific route closure | Retained prepared oracle/fallback authority until a future source idea proves one concrete row plus fallback boundary. |
| AArch64 lookup threading or private pass-context plumbing around retained lookup groups | Proof harness only; E2/private-pass-context work owns API contraction. |
| Cross-target wrapper output and wrapper compatibility rows | E4 prerequisite; wrapper behavior remains byte-stable. |
| Baseline files, expected strings, supported/unsupported markers, and helper-oracle names/status labels | Blocked by expected-string, baseline, or unsupported-path authority; not replacement evidence. |
| `PreparedFunctionLookups`, `PreparedBirModule`, aggregate lookup construction, route facades, Route 8, draft 155, E5, and broad prepared retirement | No action; separate lifecycle ownership required if reopened. |

Required proof for every accepted follow-up includes positive route/BIR-native
row evidence plus absent, invalid, duplicate/conflict, mismatch, fallback,
wrapper, printer/debug or route-debug, helper-oracle string/status/assertion,
expected-string, baseline-stability, and nearby same-feature coverage. For
target-sensitive rows, proof must also show no migration of ABI, layout,
address, move, branch, call, wrapper, final assembler, or emitted-output policy
into route/BIR diagnostic ownership.

Deferrals remain explicit: E1 owns missing semantic-helper identity proof, E2
owns public prepared API/private pass-context contraction, E4 owns
cross-target wrapper prerequisites, Route 8 remains separate for return-chain
owner/schema work, and E5/draft 155 remain unopened. Broad diagnostic/oracle
replacement and aggregate prepared retirement remain unopened.

Closeout proof: existing canonical backend regression logs were checked with
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`;
the guard passed with 180/180 before and 180/180 after, 0 new failures.
