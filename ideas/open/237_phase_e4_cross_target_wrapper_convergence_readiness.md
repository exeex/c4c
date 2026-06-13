# 237 Phase E4 cross-target wrapper convergence readiness

## Goal

Analyze which x86 or riscv wrapper/debug boundary can safely consume an
already-proven BIR route fact or retained-owner adapter while preserving
target-local ABI, frame, register, formatting, instruction-selection, emission
policy, prepared fallback, and expected strings.

This is Phase E4 of the BIR/prealloc thinning program. This activation is
analysis-only. It must produce follow-up implementation ideas for one
cross-target wrapper input, route-debug row, or wrapper-adjacent compatibility
boundary at a time; it must not directly migrate wrappers or claim broad
cross-target convergence.

## Why This Exists

Phase E0 established that cross-target wrappers remain target-local policy and
compatibility surfaces. Phase E3 then completed several diagnostic/oracle rows
and exposed/repaired x86 wrapper-adjacent blockers:

- `ideas/closed/232_phase_e3_route6_x86_scalar_i32_argument_source_route_debug_follow_up.md`
- `ideas/closed/234_phase_e3_x86_compare_join_stack_home_handoff_follow_up.md`
- `ideas/closed/235_phase_e3_route6_consumed_scalar_i32_call_argument_source_follow_up.md`
- `ideas/closed/236_phase_e3_prepared_compare_join_selected_value_chain_metadata_follow_up.md`

Those closures provide fresh evidence for x86 `ConsumedPlans`,
compare-join stack-home handoff, Route 6 consumed scalar argument-source facts,
and prepared selected-value-chain metadata. They do not prove route-wide x86
migration, riscv readiness, broad wrapper-family contraction, or prepared
aggregate retirement. E4 must decide which exact wrapper boundary, if any, is
ready for a one-surface follow-up.

## Required Inputs

This phase must read:

- `ideas/closed/217_phase_e0_prepared_bir_merge_readiness_and_ownership_boundary_audit.md`
- `docs/bir_prealloc_fusion/phase_e0_prepared_bir_merge_readiness.md`
- `ideas/closed/218_phase_e1_semantic_duplicate_candidate_triage.md`
- `ideas/closed/223_phase_e2_prepared_lookup_api_private_pass_context_readiness.md`
- `ideas/closed/226_phase_e3_prepared_diagnostic_oracle_replacement_readiness.md`
- `docs/bir_prealloc_fusion/phase_e3_prepared_diagnostic_oracle_replacement_readiness.md`
- E1/E2 closed implementation helpers `219`, `220`, `224`, and `225`
- E3 closed follow-ups `227` through `236`
- `docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md`
- `docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md`
- `docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md`
- current accepted baseline state and hook review state.

This phase must write its durable analysis payload to:

- `docs/bir_prealloc_fusion/phase_e4_cross_target_wrapper_convergence_readiness.md`

## Direction

E4 decides cross-target wrapper convergence readiness. It does not implement
the convergence itself.

For each wrapper/debug/compatibility candidate, classify it as one of:

- ready to draft one wrapper convergence implementation idea;
- needs another named AArch64 or route semantic proof first;
- needs E3 diagnostic/oracle parity first;
- retained target-local policy;
- retained prepared fallback or compatibility adapter;
- blocked by expected-string, wrapper-output, or baseline authority;
- blocked for riscv because no matching AArch64-proven semantic boundary
  exists;
- no-action.

Accepted E4 follow-up ideas must name exactly one target wrapper input,
route-debug row, handoff boundary, or wrapper-adjacent compatibility surface;
the already-proven route/BIR fact it consumes; the retained target-local
policy and prepared fallback; and the proof matrix required before wrapper
ownership can change.

## Candidate Surfaces

Start from:

- x86 Route 6 scalar `i32` argument-source route-debug row and `ConsumedPlans`
  compatibility;
- x86 compare-join stack-home handoff surface repaired by idea `234`;
- Route 6 consumed scalar `i32` call-argument source facts repaired by idea
  `235`;
- prepared selected-value-chain metadata repaired by idea `236`;
- x86 joined-branch and handoff wrapper rows adjacent to Route 5 edge/join and
  Route 7 comparison evidence;
- riscv prepared edge-publication emission and wrapper output surfaces touched
  only by no-change proof so far;
- any wrapper-adjacent row named by E0/E1/E2/E3 as a cross-target prerequisite.

## In Scope

- One x86 or riscv wrapper/debug/handoff boundary at a time.
- Consumption of an already-proven AArch64 or route-native semantic fact.
- Retained target-local frame/register/ABI/formatting/emission policy.
- Prepared fallback for absent, invalid, duplicate/conflict, mismatch,
  unsupported, and non-agreement paths.
- Byte-stable wrapper output, route-debug output, helper-oracle behavior,
  expected strings, and supported-path status.
- Follow-up implementation ideas for safe one-boundary convergence.

## Out Of Scope

- Direct implementation.
- Route-wide x86 or riscv migration.
- Inventing target-only BIR facts that bypass shared semantic ownership.
- Moving target ABI, frame layout, register allocation, formatting,
  instruction selection, emission policy, branch spelling, call wrapper policy,
  or wrapper output into target-neutral BIR.
- Broad wrapper-family contraction.
- Prepared diagnostic/oracle replacement; that belongs to E3.
- Public prepared API/private pass-context contraction; that belongs to E2.
- Aggregate `PreparedFunctionLookups`, `PreparedBirModule`, draft 155, or E5
  retirement work.
- Baseline refreshes, expected-string rewrites, helper renames, unsupported
  downgrades, timeout masking, or wrapper-output relabeling as proof.

## Expected Output

The closure note must contain:

- a link to
  `docs/bir_prealloc_fusion/phase_e4_cross_target_wrapper_convergence_readiness.md`;
- a candidate-by-candidate cross-target wrapper readiness table;
- any accepted follow-up implementation ideas, each scoped to exactly one
  wrapper/debug/handoff boundary;
- explicit retained-authority decisions for target-local policy, prepared
  fallback, diagnostics/oracles, expected strings, and baseline surfaces;
- deferrals to E1, E2, E3, Route 8, or E5 where E4 is not the correct owner;
- proof requirements for accepted implementation ideas, including positive,
  absent, invalid, duplicate/conflict, mismatch, fallback, wrapper output,
  route-debug, helper-oracle, expected-string, and baseline-stability behavior;
- an explicit statement that draft 155 / E5 and broad cross-target wrapper
  migration remain unopened.

## Reviewer Reject Signals

- Treating one x86 Route 6 proof as route-wide x86 readiness.
- Treating x86 repair work as riscv readiness.
- Creating riscv-only BIR adapters without shared AArch64 or route semantic
  ownership.
- Weakening wrapper tests, expected strings, supported-path status, helper
  names, or baselines to make convergence appear green.
- Moving target ABI/layout/register/formatting/emission policy into BIR or
  route authority.
- Opening draft 155 or claiming broad prepared retirement from E4 readiness.
