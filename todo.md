Status: Active
Source Idea Path: ideas/open/217_phase_e0_prepared_bir_merge_readiness_and_ownership_boundary_audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Build the Evidence Inventory

# Current Packet

## Just Finished

Completed Step 1 of `plan.md`: built the evidence inventory in
`docs/bir_prealloc_fusion/phase_e0_prepared_bir_merge_readiness.md`.

Inventory sources read:
`ideas/closed/181_phase_d_mir_consumer_bir_view_switch_plan.md`,
`ideas/closed/191_phase_d_followup_closure_pre_phase_e_readiness_audit.md`,
`ideas/closed/192_residual_route_view_consumer_migration_map.md`,
`ideas/closed/203_phase_d2_retained_surface_consumer_switch_analysis.md`,
`ideas/closed/207_route3_memory_source_semantic_reader.md`,
`ideas/closed/208_route3_memory_source_oracle_printer_row.md`,
`ideas/closed/209_route4_publication_source_semantic_reader.md`,
`ideas/closed/210_route4_block_entry_publication_printer_debug_row.md`,
`ideas/closed/211_route5_current_block_join_source_semantic_reader.md`,
`ideas/closed/212_route5_edge_join_oracle_printer_row.md`,
`ideas/closed/213_route6_call_source_consumer.md`,
`ideas/closed/214_route6_x86_scalar_source_route_debug_row.md`,
`ideas/closed/215_route7_comparison_provenance_consumer.md`,
`ideas/closed/216_route7_comparison_oracle_row.md`,
`docs/bir_prealloc_fusion/phase_d_followup_pre_phase_e_readiness.md`,
`docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md`,
`docs/bir_prealloc_fusion/phase_d2_retained_surface_consumer_switch_analysis.md`,
`docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md`,
`ideas/draft/155_phase_e_prepared_bir_module_retirement_plan.md`,
`src/backend/prealloc/module.hpp`, and
`src/backend/prealloc/prepared_lookups.hpp`.

Repair completed for the Step 1 inventory: draft 155 was read as required
evidence only, not activated or reworked. The durable inventory now captures
its Phase E retirement intent, required field/wrapper/compatibility questions,
construction/consumer/printer inspection requirements, expected blocker/proof
outputs, and reject signals.

Discovered `PreparedBirModule` fields: `module`, `target_profile`, `route`,
`invariants`, `names`, `control_flow`, `value_locations`, `stack_layout`,
`addressing`, `liveness`, `register_group_overrides`, `regalloc`,
`frame_plan`, `dynamic_stack_plan`, `call_plans`,
`store_source_publications`, `variadic_entry_plans`, `storage_plans`,
`i128_carriers`, `f128_carriers`, `atomic_operations`,
`intrinsic_carriers`, `inline_asm_carriers`, `f128_runtime_helpers`,
`i128_runtime_helpers`, `completed_phases`, and `notes`.

Discovered `PreparedFunctionLookups` lookup groups: `call_plans`,
`address_materializations`, `memory_accesses`, `move_bundles`,
`value_homes`, `edge_publications`, and
`edge_publication_source_producers`.

## Suggested Next

Execute Step 2: classify every `PreparedBirModule` field in
`docs/bir_prealloc_fusion/phase_e0_prepared_bir_merge_readiness.md`, keeping
BIR-owned semantics, BIR annotation candidates, target/prepared policy,
public fallback/oracle, diagnostics, cross-target boundaries, and unknowns
separate.

## Watchouts

- Step 1 found no missing required input. Draft 155 has now been read as
  evidence only; do not activate, rewrite, or execute it as the active plan.
- Phase D and D2 evidence supports selected route readers and row-scoped
  diagnostics only; it does not support prepared API deletion, route-wide
  migration, `PreparedFunctionLookups` contraction, or `PreparedBirModule`
  retirement.
- Keep target ABI/layout/register/stack/addressing/move/emission policy out of
  target-neutral BIR unless a later source names an owner and proof.
- Preserve diagnostics, printer/debug, route-debug, helper-oracle, wrapper,
  baseline, and expected-string authority as retained surfaces or blockers.
- AArch64 has selected-reader proof; x86 has only limited Route 6
  `ConsumedPlans`/route-debug reuse; riscv has no imported route-view
  migration proof.

## Proof

`printf 'docs-only Step 1 evidence inventory repair; no executable proof applies\n' > test_after.log`

Delegated proof is sufficient for this docs-only inventory slice. Proof log:
`test_after.log`.
