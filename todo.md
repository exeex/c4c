Status: Active
Source Idea Path: ideas/open/191_phase_d_followup_closure_pre_phase_e_readiness_audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Source Evidence

# Current Packet

## Just Finished

Step 1: Inventory Source Evidence completed the required readiness-audit
source inventory before drafting conclusions.

Evidence read:
- Source idea 191 and active `plan.md`; both require an analysis-only
  Phase D follow-up readiness audit and forbid implementation edits, prepared
  API contraction, or activating draft 155.
- `docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md`; key handoff
  is that selected consumer migrations are bounded route-view evidence, while
  prepared helpers remain public fallback/oracle or target-policy surfaces.
- `ideas/closed/181_phase_d_mir_consumer_bir_view_switch_plan.md`; Phase D
  created follow-up ideas 182-189 and kept return-chain separate from Routes
  1, 7, and generic aggregate thinning.
- `ideas/closed/182_phase_e_route4_publication_view_consumer_migration.md`;
  selected AArch64 dispatch-publication reader moved to a narrow Route 4
  publication identity boundary, with prepared publication helpers retained.
- `ideas/closed/183_phase_e_route5_edge_join_source_view_consumer_migration.md`;
  selected current-block join-source reader now prefers indexed Route 5 facts
  and falls back to prepared current-block join-source/oracle surfaces.
- `ideas/closed/184_phase_e_route1_producer_constant_view_consumer_migration.md`;
  selected publication-source consumer uses Route 1 producer facts where
  complete, while producer/constant/value publication and target policy
  surfaces remain prepared-owned or target-owned.
- `ideas/closed/185_phase_e_route2_select_chain_view_consumer_migration.md`;
  selected scalar ALU control-publication `select.result` path reads Route 2
  select-chain/direct-global facts first and keeps prepared helpers as fallback.
- `ideas/closed/186_phase_e_route3_memory_semantic_view_consumer_migration.md`;
  selected indirect-callee stored-value source consumer moved to Route 3
  semantic memory/source facts with prepared fallback preserved.
- `ideas/closed/187_phase_e_route6_call_use_source_view_consumer_migration.md`;
  direct-global select-chain call-argument materialization now reads Route 6
  semantic dependency facts first, while call ABI/helper/record policy remains
  target-owned.
- `ideas/closed/188_phase_e_route7_comparison_view_consumer_migration.md`;
  selected materialized-condition branch consumer prefers validated Route 7
  provenance and preserves prepared/emitted-condition fallback behavior.
- `ideas/closed/189_phase_e_cross_target_route_view_interface_reuse.md`;
  x86 `ConsumedPlans` reuses the AArch64-proven Route 6 call-use source view
  only when route and prepared call-plan source ids agree, preserving prepared
  fallback and target-local x86 policy.
- `ideas/closed/190_full_suite_baseline_99_percent_regression_attribution.md`;
  accepted baseline repair identifies Route 3 FP global-load identity as the
  first-bad semantic/prepared-policy boundary and restores prepared fallback
  without weakening c_testsuite contracts.
- `ideas/draft/155_phase_e_prepared_bir_module_retirement_plan.md`; true
  Phase E is still draft analysis for field-by-field `PreparedBirModule`
  retirement and requires Phase A-D evidence before opening.
- `docs/bir_prealloc_fusion/return_chain_owner_schema_decision.md` and
  return-chain ideas 176-180; return-chain has its own BIR-owned identity
  route, oracle equivalence, AArch64 consumer migration, and prepared API
  contraction history, and must not be folded into Route 1, Route 7, or a
  generic aggregate retirement claim.

Missing material or evidence gaps:
- `docs/bir_prealloc_fusion/phase_d_followup_pre_phase_e_readiness.md` does
  not exist yet; this is expected before Step 3 and should not be treated as a
  Step 1 blocker.
- Closed idea numbering is ambiguous around `177`: both
  `ideas/closed/177_aarch64_selected_f64_global_readback_dispatch_debt.md` and
  `ideas/closed/177_bir_return_chain_schema_index.md` exist. For the
  return-chain inventory, cite `177_bir_return_chain_schema_index.md`; the
  dispatch-debt file is unrelated close-level AArch64 debt.
- No required source material from the Step 1 read list was missing.

Accepted baseline evidence:
- `test_baseline.log` is the accepted full-suite baseline at commit
  `e5218942517a2bb9c3bd0167d810e27bed8273c8` with subject
  `restore FP global-load prepared fallback`.
- Baseline regex is `<full-suite>`.
- The recorded result is 3428/3428 tests passing, 0 failures, 100% passed, with
  total real test time 40.74 seconds.
- Later audit steps should cite this baseline together with idea 190's rule:
  Route 3 semantic memory/source identity does not replace AArch64 prepared
  target-addressing or materialization policy.

## Suggested Next

Execute Step 2 from `plan.md`: build the route and boundary tables from the
inventoried evidence, classifying each idea 182-189 by selected consumer,
route view or facade, prepared fallback/oracle retained, target-policy
boundary, proof scope, and residual prepared readers.

## Watchouts

- This is analysis-only; do not edit implementation files or start draft 155.
- Treat ideas 182-189 as Phase D follow-up slices, not true Phase E retirement.
- Preserve prepared fallback/oracle and target-policy boundaries in the audit.
- Reflect idea 190's Route 3 finding as a readiness rule or blocker.
- Keep source idea edits unnecessary unless durable intent changes.
- Import return-chain as its own completed owner/schema and contraction line,
  not as generic Route 1/Route 7/aggregate evidence.
- Do not cite the duplicate `177_aarch64_selected_f64_global_readback_dispatch_debt.md`
  as a return-chain follow-up.

## Proof

Docs-only/lifecycle-only packet; no build or CTest run required by the
delegated proof. Verified by reading the required Step 1 source material and
accepted `test_baseline.log`; no new `test_after.log` was produced for this
documentation inventory.
