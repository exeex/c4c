# Phase D versus Phase E Lifecycle Naming Cleanup

Status: naming note complete for future Phase E readiness work.

Ideas 182-189 are Phase D follow-up slices even though their historical closed
idea filenames use `phase_e_*`. Treat those filenames as review history, not
as proof that the true Phase E `PreparedBirModule` retirement analysis has
run. No closed files were renamed by this cleanup; the historical filenames are
preserved.

Draft 155, `ideas/draft/155_phase_e_prepared_bir_module_retirement_plan.md`,
remains the future true Phase E `PreparedBirModule` retirement analysis. It
must not be cited as activated, completed, or made safe by the label cleanup
below.

## Classification

Use "Phase D follow-up selected-consumer migration with a Phase E-like
filename" for ideas 182-188. Use "Phase D follow-up cross-target interface
reuse with a Phase E-like filename" for idea 189.

| Idea | Route or boundary | Actual accepted scope | Retained or not-proven scope | Recommended future citation wording |
| --- | --- | --- | --- | --- |
| 182 | Route 4 publication | One selected AArch64 dispatch-publication reader, `current_block_entry_publication_register(...)`, reads a narrow Route 4 publication-identity boundary with prepared fallback and oracle behavior preserved. | Route-wide publication migration, prepared publication helper contraction, x86/riscv wrapper replacement, value-home/storage/move policy, and printer/debug/oracle replacement. | "Idea 182 was a Phase D follow-up Route 4 selected-consumer migration with a Phase E-like filename." |
| 183 | Route 5 edge/join-source | One selected AArch64 current-block join-source reader behind `build_current_block_join_prepared_query_routing(...)` prefers the indexed Route 5 current-block join-source view when valid BIR data exists. | Edge-copy and join parallel-copy consumer families beyond the selected reader, move-bundle policy, prepared edge-publication helper contraction, and broad aggregate contraction. | "Idea 183 was a Phase D follow-up Route 5 selected-consumer migration with a Phase E-like filename." |
| 184 | Route 1 producer/constant | One selected publication-source consumer, `value_publication_may_read_register_index(...)`, prefers the Route 1 publication producer view for complete same-block source-producer facts. | Route-wide scalar materialization, producer/constant, value publication, ALU, memory/call operand, target register/home/storage/move policy, machine-record policy, and API contraction. | "Idea 184 was a Phase D follow-up Route 1 selected-consumer migration with a Phase E-like filename." |
| 185 | Route 2 select-chain/direct-global | One selected AArch64 scalar ALU control-publication `select.result` path in `lower_scalar_select_publication(...)` consults the local Route 2 adapter first. | Remaining select-chain/direct-global publication, call, memory, ALU, FP, producer, edge-copy, printer/test, and oracle consumers; target materialization and storage policy. | "Idea 185 was a Phase D follow-up Route 2 selected-consumer migration with a Phase E-like filename." |
| 186 | Route 3 memory/source | One selected AArch64 indirect-callee stored-value source consumer reads the narrow Route 3 stored-source view with prepared fallback preserved. | Direct memory/addressing, globals, load-local, FP/global-load materialization, store/publication writeback, printer/oracle, and target-addressing or operand-formation policy. | "Idea 186 was a Phase D follow-up Route 3 selected-consumer migration with a Phase E-like filename." |
| 187 | Route 6 call-use source | One direct-global select-chain call-argument materialization consumer reads Route 6 semantic dependency facts first while retaining prepared publication-source routing fallback/oracle behavior. | Other call argument/result role classes, publication-source classes, recursive operand fallback, ABI/helper/carrier/call-record policy, prepared call surfaces, and x86/riscv wrappers. | "Idea 187 was a Phase D follow-up Route 6 selected-consumer migration with a Phase E-like filename." |
| 188 | Route 7 comparison/condition | One selected AArch64 materialized-condition branch consumer, `lower_materialized_compare_condition_branch(...)`, prefers validated Route 7 materialized-condition provenance. | Other fused-compare, scalar comparison, branch operand, ALU, return-chain-adjacent, printer/debug, and oracle consumers; branch spelling, condition-code, hazard, emitted-register, and ALU record policy. | "Idea 188 was a Phase D follow-up Route 7 selected-consumer migration with a Phase E-like filename." |
| 189 | Cross-target Route 6 interface reuse | One x86 `ConsumedPlans` call-boundary selector reuses the AArch64-proven Route 6 call-use source view for scalar named `i32` `ArgumentValue` facts when route and prepared call-plan source ids agree. | Broad x86 readiness, any riscv readiness, x86/riscv wrapper migration, target-policy migration, route-wide cross-target migration, and prepared aggregate contraction. | "Idea 189 was Phase D follow-up x86 Route 6 interface reuse with a Phase E-like filename." |

## Draft 155 Prerequisites

Before draft 155 is activated or rewritten into an active plan, cite this note
and the prerequisite evidence set below. These documents support future
planning; they do not by themselves authorize `PreparedBirModule` retirement,
prepared API contraction, route-wide migration, broad x86/riscv readiness, or
target-policy migration.

- `docs/bir_prealloc_fusion/phase_d_followup_pre_phase_e_readiness.md`
- `docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md`
- `docs/bir_prealloc_fusion/cross_target_route_view_reuse_map.md`
- `docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md`
- `docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md`
- `docs/bir_prealloc_fusion/return_chain_import_and_naming.md`
- Background docs: `phase_a_normalization_candidates.md`,
  `phase_b_annotation_schema_candidates.md`,
  `phase_c_private_cache_contraction.md`,
  `phase_d_mir_consumer_switch_plan.md`, and
  `return_chain_owner_schema_decision.md`.

Draft 155 still needs its own field-by-field `PreparedBirModule` and
`PreparedFunctionLookups` ownership analysis, residual consumer map, diagnostic
and oracle replacement strategy, compatibility plan, and final proof strategy.

## Stable Citation Rules

Accepted wording:

- "Phase D follow-up selected-consumer migration with a Phase E-like filename."
- "Phase D follow-up evidence for one selected Route N consumer boundary,
  stored under a historical `phase_e_*` closed idea name."
- "Ideas 182-188 are useful inputs for future Phase E planning, but they are
  not the Phase E retirement plan."
- "Idea 189 proves one x86 Route 6 scalar call-use source interface reuse
  point after AArch64 Route 6 proof, with prepared fallback for absent or
  mismatched facts."

Use this note whenever future plans, audits, reviews, or closure notes refer
to ideas 182-189 as a group, mention their `phase_e_*` filenames, or could let
"Phase E" be misread as completed `PreparedBirModule` retirement.

Reject-language rules:

- Do not call ideas 182-189 true Phase E retirement, Phase E retirement
  completion, the completed Phase E plan, or draft 155 execution.
- Do not describe them as `PreparedBirModule` contraction, demotion, deletion,
  broad aggregate replacement, BIR-owned clone work, or field-level ownership
  resolution.
- Do not say they complete draft 155, make draft 155 ready, or automatically
  unblock draft 155 activation.
- Do not call ideas 182-188 route-wide migrations, complete Route 1-7
  migration, residual-consumer exhaustion, prepared API deletion, prepared
  helper removal, target-policy migration, or oracle/printer/debug retirement.
- Do not call idea 189 broad x86 readiness, broad cross-target readiness,
  riscv readiness, x86/riscv wrapper migration, or target-policy migration.
- Do not cite this naming cleanup as implementation progress, test expectation
  progress, unsupported-marker progress, or permission to weaken prepared
  fallback, diagnostics, target-wrapper, or oracle contracts.
