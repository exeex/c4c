Status: Active
Source Idea Path: ideas/open/218_phase_e1_semantic_duplicate_candidate_triage.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Build the E1 Evidence Inventory

# Current Packet

## Just Finished

Completed Step 1 of `plan.md`: built the E1 evidence inventory and created
`docs/bir_prealloc_fusion/phase_e1_semantic_duplicate_candidate_triage.md` as
the durable triage skeleton.

Complete input inventory recorded for this packet:

- `ideas/closed/217_phase_e0_prepared_bir_merge_readiness_and_ownership_boundary_audit.md`
- `docs/bir_prealloc_fusion/phase_e0_prepared_bir_merge_readiness.md`
- `ideas/closed/181_phase_d_mir_consumer_bir_view_switch_plan.md`
- `ideas/closed/203_phase_d2_retained_surface_consumer_switch_analysis.md`
- `docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md`
- `docs/bir_prealloc_fusion/phase_d2_retained_surface_consumer_switch_analysis.md`
- `docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md`
- Route 3 through Route 7 closure notes:
  `ideas/closed/207_route3_memory_source_semantic_reader.md`,
  `ideas/closed/208_route3_memory_source_oracle_printer_row.md`,
  `ideas/closed/209_route4_publication_source_semantic_reader.md`,
  `ideas/closed/210_route4_block_entry_publication_printer_debug_row.md`,
  `ideas/closed/211_route5_current_block_join_source_semantic_reader.md`,
  `ideas/closed/212_route5_edge_join_oracle_printer_row.md`,
  `ideas/closed/213_route6_call_source_consumer.md`,
  `ideas/closed/214_route6_x86_scalar_source_route_debug_row.md`,
  `ideas/closed/215_route7_comparison_provenance_consumer.md`, and
  `ideas/closed/216_route7_comparison_oracle_row.md`
- `src/backend/prealloc/module.hpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- current accepted baseline state, as captured by recent D2/E0 closure notes:
  accepted baseline and string authority are guardrails; prior accepted
  full-suite state was restored to 3428/3428, and E0 close proof used matching
  string-authority logs.

Missing required inputs: none found.

Ambiguous evidence: E0 names the six E1 candidate families but does not make
any family implementation-ready. Step 2 still needs exact helper/API surfaces,
semantic owners, retained prepared fallback or policy, public consumers, and
proof shapes before readiness buckets are assigned.

## Suggested Next

Execute Step 2: classify the six E0 E1 candidate families in
`docs/bir_prealloc_fusion/phase_e1_semantic_duplicate_candidate_triage.md`,
keeping every classification evidence-backed and avoiding implementation
permission from the E0 category names alone.

## Watchouts

- This active idea is analysis-only.
- Do not edit implementation files during Phase E1 triage.
- Do not treat E0 candidate categories as implementation permission.
- Do not open broad prepared retirement, aggregate `PreparedBirModule` /
  `PreparedFunctionLookups` retirement, or draft 155 / E5 work.
- Keep follow-up ideas scoped to one semantic duplicate helper family or one
  row-scoped semantic/oracle surface.
- Preserve target/prepared policy, fallback/oracle, diagnostics, helper-oracle
  names, supported-path status, baselines, and expected strings.
- The Step 1 skeleton deliberately marks all six families pending Step 2; do
  not infer a readiness bucket from the table order.

## Proof

Docs-only proof selected by supervisor; no build was required and no
`test_after.log` was written for this packet because the proof command is a
document existence/content check and the packet forbids touching canonical log
files.

Command:

```sh
test -f docs/bir_prealloc_fusion/phase_e1_semantic_duplicate_candidate_triage.md && rg -n "aggregate BIR semantic forwarding|control-flow identity helpers|route source/publication/join/call/comparison identity helpers|liveness identity helpers|intrinsic metadata adapters|row-scoped diagnostic/oracle helpers" docs/bir_prealloc_fusion/phase_e1_semantic_duplicate_candidate_triage.md
```

Result: passed. The command found all six required candidate-family names in
the durable E1 document at lines 98-103.
