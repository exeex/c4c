Status: Active
Source Idea Path: ideas/open/239_phase_e5_prepared_bir_module_demotion_or_retirement_gate.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Build the E5 Evidence Inventory

# Current Packet

## Just Finished

Step 1 built the E5 evidence inventory for idea 239 from existing durable
artifacts and lifecycle state. No implementation files, `plan.md`, source
idea, or E5 document edits were made.

Prerequisite inventory:

- E0 is closed in
  `ideas/closed/217_phase_e0_prepared_bir_merge_readiness_and_ownership_boundary_audit.md`;
  durable payload
  `docs/bir_prealloc_fusion/phase_e0_prepared_bir_merge_readiness.md`
  contains the field-by-field `PreparedBirModule` ownership map, the
  `PreparedFunctionLookups` group map, E1-E5 readiness synthesis, draft 155 /
  E5 rewrite criteria, and retained no-reduction surfaces. It keeps draft 155
  unopened and rejects broad prepared retirement from selected route readers.
- E1 is closed in
  `ideas/closed/218_phase_e1_semantic_duplicate_candidate_triage.md`;
  durable payload
  `docs/bir_prealloc_fusion/phase_e1_semantic_duplicate_candidate_triage.md`
  accepted only the control-flow identity helper and Route 1-7 identity helper
  families for later one-helper ideas, and deferred aggregate forwarding,
  liveness, intrinsic metadata, row-scoped diagnostics/oracles, Route 8, draft
  155, and E5. Accepted E1 follow-ups `219` and `220` are now closed.
- E2 is closed in
  `ideas/closed/223_phase_e2_prepared_lookup_api_private_pass_context_readiness.md`;
  durable payload
  `docs/bir_prealloc_fusion/phase_e2_prepared_lookup_api_private_pass_context_readiness.md`
  accepted only the two proven E1 helper families as public API/private
  pass-context follow-ups. Accepted E2 follow-ups `224` and `225` are now
  closed. All lookup groups, aggregate lookup construction, draft 155 / E5,
  and aggregate `PreparedFunctionLookups` retirement remain unopened.
- E3 is closed in
  `ideas/closed/226_phase_e3_prepared_diagnostic_oracle_replacement_readiness.md`;
  durable payload
  `docs/bir_prealloc_fusion/phase_e3_prepared_diagnostic_oracle_replacement_readiness.md`
  accepted row-scoped diagnostic/oracle follow-ups `227` through `233`, all
  now closed, and records retained authority for non-agreement fallback,
  target policy, wrappers, expected strings, baselines, aggregate prepared
  APIs, Route 8, draft 155, and E5. Additional closed E3 follow-ups `234`,
  `235`, and `236` are E4 inputs but do not by themselves prove wrapper or
  aggregate retirement readiness.
- E4 is closed in
  `ideas/closed/237_phase_e4_cross_target_wrapper_convergence_readiness.md`;
  durable payload
  `docs/bir_prealloc_fusion/phase_e4_cross_target_wrapper_convergence_readiness.md`
  accepted exactly one x86-only wrapper/debug compatibility follow-up and
  recorded no riscv readiness, no broad x86 or cross-target wrapper migration,
  no aggregate prepared retirement, and no draft 155 / E5 opening.

Idea 238 status:

- Idea 238 is closed at
  `ideas/closed/238_phase_e4_x86_route6_scalar_i32_route_debug_consumedplans_compatibility_follow_up.md`.
- Its closure records that the selected x86 Route 6 scalar `i32`
  argument-source route-debug positive path now consumes the Route 6
  source-record agreement helper while x86 still owns the prepared
  `source_value_id` / `ConsumedPlans` compatibility check and target-local
  call-wrapper policy.
- 238 is therefore no longer an open prerequisite blocker for E5, but its
  evidence remains narrow: it does not prove broad x86 call-wrapper migration,
  riscv readiness, route-wide wrapper convergence, aggregate prepared
  retirement, or transfer of ABI/frame/register/formatting/emission/call
  policy into Route 6 or BIR.

Current blocker list for demotion or retirement claims:

- Whole `PreparedBirModule` retirement remains blocked by aggregate
  compatibility/pass-context delivery, public prepared consumers,
  printer/debug visibility, fallback/oracle authority, cross-target wrappers,
  target-policy products, and baseline/string authority.
- Whole `PreparedFunctionLookups` retirement remains blocked because its
  lookup groups still mix selected semantic subfacts with public construction,
  pass-context delivery, wrappers, diagnostics/oracles, fallback, and
  target/prepared policy.
- riscv readiness remains blocked; E4 records no matching AArch64-proven
  semantic boundary plus riscv formatting/emission/output no-change proof.
- x86 wrapper readiness is limited to the closed 238 Route 6 scalar `i32`
  route-debug / `ConsumedPlans` compatibility row; adjacent joined-branch,
  handoff, call-wrapper, and wrapper-family migration remains blocked or
  requires another exact named proof.
- Route 8 return-chain ownership remains separate and unavailable as proof for
  Route 1-7, wrapper, aggregate lookup, or prepared aggregate retirement.
- Draft 155 exists only as `ideas/draft/155_phase_e_prepared_bir_module_retirement_plan.md`;
  the unrelated closed `ideas/closed/155_bir_block_entry_publication_identity.md`
  is not the draft retirement plan. No current artifact opens draft 155.
- Any field group lacking cross-target, diagnostic/oracle, fallback, baseline,
  or public-consumer proof must be classified as blocked or retained in Step 2.

Baseline and string-authority evidence to preserve:

- E0 close proof used matching `string_authority_guard` logs with 1/1 before
  and after; E1 close proof reused matching 1/1 string-authority logs.
- E2 close proof used matching two-test backend logs with 2/2 before and
  after. E3 close proof used canonical backend regression logs with 180/180
  before and after and no new failures.
- E4 records the accepted full-suite baseline as 3428/3428 at commit
  `8cebab4beba219e6a8cdef998bc970c8658ce28b`, plus later 15-test
  prepared/prealloc/x86 close-scope proof. Idea 238 closure records narrow
  regression guard success for `backend_prepared_lookup_helper` and
  `backend_x86_route_debug` at 2/2, broader x86 backend validation at
  182/182, and route-quality review finding no testcase-overfit or broad
  x86/riscv readiness claim.
- Step 2 must preserve helper-oracle names/status labels, expected strings,
  supported-path status, printer/debug output, route-debug output, wrapper
  output, fallback behavior, and baselines. Baseline refreshes, expectation
  rewrites, helper renames, supported-path downgrades, timeout masking,
  wrapper-output relabeling, facade-only moves, and named-case shortcuts are
  not valid readiness evidence.

Exact artifact set Step 2 should classify:

- `ideas/open/239_phase_e5_prepared_bir_module_demotion_or_retirement_gate.md`
- `plan.md`
- `todo.md`
- `docs/bir_prealloc_fusion/phase_e0_prepared_bir_merge_readiness.md`
- `docs/bir_prealloc_fusion/phase_e1_semantic_duplicate_candidate_triage.md`
- `docs/bir_prealloc_fusion/phase_e2_prepared_lookup_api_private_pass_context_readiness.md`
- `docs/bir_prealloc_fusion/phase_e3_prepared_diagnostic_oracle_replacement_readiness.md`
- `docs/bir_prealloc_fusion/phase_e4_cross_target_wrapper_convergence_readiness.md`
- `docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md`
- `docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md`
- `ideas/draft/155_phase_e_prepared_bir_module_retirement_plan.md`
- Phase analysis closure notes: `ideas/closed/217_phase_e0_prepared_bir_merge_readiness_and_ownership_boundary_audit.md`,
  `ideas/closed/218_phase_e1_semantic_duplicate_candidate_triage.md`,
  `ideas/closed/223_phase_e2_prepared_lookup_api_private_pass_context_readiness.md`,
  `ideas/closed/226_phase_e3_prepared_diagnostic_oracle_replacement_readiness.md`,
  and `ideas/closed/237_phase_e4_cross_target_wrapper_convergence_readiness.md`.
- E1/E2 implementation closure notes: `ideas/closed/219_phase_e1_control_flow_identity_helper_contraction.md`,
  `ideas/closed/220_phase_e1_route_identity_helper_contraction.md`,
  `ideas/closed/224_phase_e2_control_flow_branch_target_helper_private_pass_context.md`,
  and `ideas/closed/225_phase_e2_fused_compare_operand_producer_helper_private_pass_context.md`.
- E3/E4 follow-up closure notes: `ideas/closed/227_phase_e3_branch_target_helper_oracle_follow_up.md`,
  `ideas/closed/228_phase_e3_fused_compare_operand_producer_helper_oracle_follow_up.md`,
  `ideas/closed/229_phase_e3_route3_memory_source_stored_value_helper_oracle_follow_up.md`,
  `ideas/closed/230_phase_e3_route4_block_entry_publication_printer_debug_follow_up.md`,
  `ideas/closed/231_phase_e3_route5_current_block_join_source_helper_oracle_follow_up.md`,
  `ideas/closed/232_phase_e3_route6_x86_scalar_i32_argument_source_route_debug_follow_up.md`,
  `ideas/closed/233_phase_e3_route7_materialized_condition_helper_oracle_follow_up.md`,
  `ideas/closed/234_phase_e3_x86_compare_join_stack_home_handoff_follow_up.md`,
  `ideas/closed/235_phase_e3_route6_consumed_scalar_i32_call_argument_source_follow_up.md`,
  `ideas/closed/236_phase_e3_prepared_compare_join_selected_value_chain_metadata_follow_up.md`,
  and `ideas/closed/238_phase_e4_x86_route6_scalar_i32_route_debug_consumedplans_compatibility_follow_up.md`.

## Suggested Next

Execute Step 2 by mapping `PreparedBirModule` and `PreparedFunctionLookups`
field groups from the exact artifact set above, classifying each group as
demotion-ready, target/prepared policy, private pass context, public
fallback/oracle, diagnostic/string authority, cross-target compatibility, or
blocked/retained for missing proof.

## Watchouts

- Treat closed idea 238 as prerequisite complete, but only for the single x86
  Route 6 scalar `i32` route-debug / `ConsumedPlans` compatibility boundary.
- Do not use E1/E2 selected-helper closures, E3 row closures, or E4 x86-only
  wrapper evidence as proof of whole-module retirement, lookup-group deletion,
  riscv readiness, or broad wrapper migration.
- Keep target policy out of target-neutral BIR: ABI, frame, registers, stack,
  address formation, move scheduling, call/helper protocols, branch spelling,
  suffix/fused legality, final assembler, emission order, and wrapper output
  remain target/prepared-owned unless a later source proves a narrower owner.
- Preserve public fallback/oracle, diagnostic/string, expected-string,
  supported-path, printer/debug, route-debug, wrapper-output, and baseline
  authority. Expectation rewrites, helper renames, unsupported downgrades,
  baseline refreshes, facade moves, and named-case-only shortcuts are reject
  signals.

## Proof

Docs/lifecycle-only Step 1 inventory packet. No implementation files changed,
so no build or CTest proof was required and no `test_after.log` was generated
for this packet.
