# Phase E0 Prepared/BIR Merge Readiness

Status: Step 1 evidence inventory complete.

Source idea: `ideas/open/217_phase_e0_prepared_bir_merge_readiness_and_ownership_boundary_audit.md`

## Evidence Inventory

This document is the durable Phase E0 analysis surface for the future
Prepared/BIR merge readiness map. Step 1 is inventory only: it records the
inputs, structural surfaces, and retained-boundary evidence that later steps
must classify. It does not implement, delete, privatize, rename, or weaken
prepared APIs, diagnostics, tests, or target behavior.

### Sources Read

The Step 1 inventory used these required inputs:

| Source | Evidence captured |
| --- | --- |
| `ideas/closed/181_phase_d_mir_consumer_bir_view_switch_plan.md` | Phase D direction: migrate one semantic consumer group at a time, keep prepared answers as oracles until equivalence is proven, keep target/layout/codegen policy outside BIR, and treat return-chain as a separate owner/schema line. |
| `ideas/closed/191_phase_d_followup_closure_pre_phase_e_readiness_audit.md` | Ideas 182-189 are Phase D follow-up slices, not true Phase E retirement. Draft 155 was not ready because field ownership, fallback/oracle, target-policy, cross-target, and return-chain blockers remained. |
| `ideas/closed/192_residual_route_view_consumer_migration_map.md` | Route 1-7 residual production, printer/debug, target-wrapper, oracle/test, and retained-policy consumers remain after selected readers moved. |
| `ideas/closed/203_phase_d2_retained_surface_consumer_switch_analysis.md` | D2 retained surfaces remain one-surface or one-row follow-up candidates only; aggregate `PreparedFunctionLookups`, aggregate `PreparedBirModule`, broad wrappers, baseline/string authority, and return-chain contraction are no-action. |
| `ideas/closed/207_route3_memory_source_semantic_reader.md` | Route 3 can supply selected memory/source identity only under route/prepared agreement; address formation, relocation, addressing legality, materialization, final operands, and target policy remain prepared/target-owned. |
| `ideas/closed/208_route3_memory_source_oracle_printer_row.md` | Route 3 diagnostic replacement is row-scoped; prepared diagnostics remain authoritative for fallback, non-agreement, ambiguity, mismatch, and target-policy-sensitive output. |
| `ideas/closed/209_route4_publication_source_semantic_reader.md` | Route 4 can identify a selected publication source only with prepared publication fallback for absent, invalid, ambiguous, and mismatched facts. |
| `ideas/closed/210_route4_block_entry_publication_printer_debug_row.md` | Route 4 printer/debug replacement is one byte-stable block-entry row with prepared agreement and fail-closed fallback; wrapper and publication mechanics remain retained. |
| `ideas/closed/211_route5_current_block_join_source_semantic_reader.md` | Route 5 can supply one current-block join-source identity under agreement; join, edge, move-bundle, branch, parallel-copy, output, and wrapper behavior remain prepared/target-owned. |
| `ideas/closed/212_route5_edge_join_oracle_printer_row.md` | Route 5 diagnostic metadata is retained row-scoped evidence only; absent, invalid, duplicate/conflict, memory-source, mismatch, unsupported, branch/parallel-copy, printer, and wrapper paths stay retained. |
| `ideas/closed/213_route6_call_source_consumer.md` | Route 6 can supply one call argument/result source under agreement; ABI, wrapper, aggregate transport, call records, printer behavior, storage, movement, and emitted behavior remain prepared/target-owned. |
| `ideas/closed/214_route6_x86_scalar_source_route_debug_row.md` | One x86 Route 6 scalar source route-debug row is proven through `ConsumedPlans`; this is not broad x86 call-wrapper migration or `ConsumedPlans` removal. |
| `ideas/closed/215_route7_comparison_provenance_consumer.md` | Route 7 can supply one comparison provenance consumer under agreement; branch behavior, suffixes, fused legality, final assembler, and expected-string output remain AArch64/prepared-owned. |
| `ideas/closed/216_route7_comparison_oracle_row.md` | One Route 7 materialized-condition helper-oracle row can use route-native evidence while preserving prepared oracle assertions, fallback, wrappers, branch-control output, and expected strings. |
| `docs/bir_prealloc_fusion/phase_d_followup_pre_phase_e_readiness.md` | Selected-consumer migrations are useful evidence but not prepared API deletion, route-wide migration, or aggregate contraction. It records retained fallback/oracle, target-policy, cross-target, return-chain, and draft-readiness blockers. |
| `docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md` | Route 1-7 map of selected-reader evidence, residual consumers, required route facts, fallback/oracle surfaces, and proof expectations. |
| `docs/bir_prealloc_fusion/phase_d2_retained_surface_consumer_switch_analysis.md` | Retained-surface dependency map, one-surface follow-up decisions, no-action decisions for aggregates/pass-context/wrappers/baseline/Route 8, and current consumer anchors. |
| `docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md` | Lookup group ownership map: route-native subfacts exist inside some groups, but no lookup group is deletion-, privatization-, or aggregate-replacement-ready. |
| `ideas/draft/155_phase_e_prepared_bir_module_retirement_plan.md` | Draft Phase E intent: stop treating `PreparedBirModule` as a second IR only after BIR normalization, annotation schema, cache contraction, and MIR consumer migration have enough coverage; answer field durability, BIR annotation, private pass-context, wrapper, and compatibility questions; inspect construction, mutation, consumers, printer/dump tooling, and Phase A-D proof; reject premature removal, renamed-wrapper hiding, lost diagnostics, and unrelated backend feature bundling. |
| `src/backend/prealloc/module.hpp` | Structural inventory of `PreparedBirModule` fields and public inline lookup/helper surfaces. |
| `src/backend/prealloc/prepared_lookups.hpp` | Structural inventory of `PreparedFunctionLookups` lookup groups and construction entry points. |

Draft 155 was read as an evidence input only. This inventory does not activate,
rewrite, or execute that draft; it uses the draft to preserve the intended
Phase E retirement criteria and reject signals while this Phase E0 pass maps
the safer ownership boundaries first.

### PreparedBirModule Field Inventory

`PreparedBirModule` currently aggregates target-neutral BIR, target profile and
route metadata, prepared semantic/policy products, target-specific helper
plans, diagnostics, and phase notes:

| Field | Raw surface captured for later classification |
| --- | --- |
| `module` | BIR module payload; the later map must separate BIR-owned semantic state from prepared wrapper compatibility. |
| `target_profile` | Target ABI/layout/profile policy input; retained target/prepared policy unless a later source names a target-neutral replacement. |
| `route` | Preparation route selection/defaulting surface; likely pass-context/diagnostic metadata until classified. |
| `invariants` | Prepared invariant diagnostics/validation state; diagnostic/oracle authority must remain visible. |
| `names` | Prepared name tables and id/string bridge; may support BIR identity lookup but also diagnostics and public compatibility. |
| `control_flow` | Prepared control-flow products consumed by lowering and lookup construction; classify semantic CFG facts separately from target traversal/policy. |
| `value_locations` | Value location/home information; target/prepared policy for homes, registers, stack slots, storage, and rendering remains retained. |
| `stack_layout` | Stack/frame layout policy; retained target/prepared policy. |
| `addressing` | Address materialization and target addressing policy; retained target/prepared policy. |
| `liveness` | Prepared liveness/pass state; classify as BIR annotation candidate or transient pass context only with consumer proof. |
| `register_group_overrides` | Register group override policy; retained target/prepared policy unless later proof isolates pure semantic annotation. |
| `regalloc` | Register allocation state; retained target/prepared policy. |
| `frame_plan` | Frame plan policy; retained target/prepared policy. |
| `dynamic_stack_plan` | Dynamic stack plan policy; retained target/prepared policy. |
| `call_plans` | Call ABI/layout/helper plans; Route 6 subfacts may be semantic, but whole plans remain target/prepared policy and fallback/oracle. |
| `store_source_publications` | Store-source publication planning; overlaps Routes 1/2/3/4/6 source facts but still includes publication/materialization policy and fallback surfaces. |
| `variadic_entry_plans` | Variadic entry ABI/helper policy; retained target/prepared policy. |
| `storage_plans` | Storage/home/encoding policy; retained target/prepared policy. |
| `i128_carriers` | Wide-value carrier protocol; retained target/prepared or helper policy. |
| `f128_carriers` | Wide-value carrier protocol; retained target/prepared or helper policy. |
| `atomic_operations` | Atomic operation lowering/helper policy; retained target/prepared policy unless a semantic-only reader is later proven. |
| `intrinsic_carriers` | Intrinsic carrier/helper protocol; retained target/prepared/helper policy. |
| `inline_asm_carriers` | Inline asm carrier/protocol state; retained target/prepared policy. |
| `f128_runtime_helpers` | Runtime helper policy/protocol; retained target/prepared/helper policy. |
| `i128_runtime_helpers` | Runtime helper policy/protocol; retained target/prepared/helper policy. |
| `completed_phases` | Phase/debug metadata; diagnostic/pass-context surface. |
| `notes` | Prepared notes/diagnostics; diagnostic/string authority. |

The header also exposes public inline lookup helpers over the aggregate, such
as register-group overrides, addressing, value-location functions, frame and
dynamic-stack plans, call plans, variadic helper operand homes, storage plans,
wide carriers, atomics, intrinsics, inline asm, and runtime helpers. Later
steps must treat these as public prepared surfaces until every production,
wrapper, printer/debug, and oracle consumer has a replacement or retained-owner
decision.

### PreparedFunctionLookups Group Inventory

`PreparedFunctionLookups` currently groups seven per-function lookup families:

| Lookup group | Raw surface captured for later classification |
| --- | --- |
| `call_plans` | Call-plan lookups. Route 6 can replace selected source identity reads, but ABI placement, helper/carrier protocol, result lanes, call records, and call-boundary oracles stay retained. |
| `address_materializations` | Address-materialization lookups. Current evidence classifies this as target/prepared policy: frame/stack/global/TLS relocation, offsets, base-plus-offset legality, volatile/address-space behavior, and final operands are not route facts. |
| `memory_accesses` | Memory access lookups. Route 3 may own selected memory/source identity, but prepared memory/addressing fallback and target policy remain retained. |
| `move_bundles` | Move-bundle lookups. Route 5/6 may provide adjacent source identity, but scheduling, homes, storage, cycle temporaries, ABI move phases, and final move records stay prepared/target-owned. |
| `value_homes` | Value-home lookups. Evidence retains prepared/target ownership for storage, register choice, stack slot/offset, rematerialization spelling, rendering, and home policy. |
| `edge_publications` | Edge-publication lookups. Routes 4/5 may own selected publication or edge/join identity, while publication construction, block order, moves/homes/storage, stack-source extension, wrappers, and final records remain retained. |
| `edge_publication_source_producers` | Source-producer lookups. Route 1, 2, 5, 6, and 7 subfacts may be semantic candidates one consumer at a time; printer/debug, route-debug, helper-oracle, materialization, call/publication routing, and final records remain retained. |

The only construction entry points in the header are
`make_prepared_function_lookups(...)` and
`make_prepared_move_bundle_lookups(...)`. This supports the current
classification of the aggregate as compatibility/pass-context delivery rather
than a deletion-ready owner boundary.

### Phase D, D2, And Route 1-8 Constraints

Phase D constraints:

- Selected consumer migrations prove bounded route-view use, not route-wide
  migration, prepared API deletion, or `PreparedBirModule` retirement.
- Prepared answers remain public fallback/oracle surfaces until route/prepared
  equivalence and replacement diagnostics are proven.
- Target/layout/codegen policy stays outside BIR: homes, registers, stack
  slots, ABI placement, move scheduling, branch spelling, fused legality,
  emitted storage, and final instruction order are not semantic route facts.
- Route index/facade work is not a generic `PreparedFunctionLookups` clone.
- Return-chain is separate owner/schema evidence and must not be folded into
  Route 1 producer identity, Route 7 comparison provenance, predecessor
  rescans, name matching, or a generic route-index facade.

Phase D2 constraints:

- No C2/D2 retained surface is broad contraction-ready.
- Future implementation work must be one prepared surface or one
  diagnostic/oracle row wide.
- Aggregate `PreparedFunctionLookups`, aggregate `PreparedBirModule`, AArch64
  lookup threading/pass-context, broad x86/riscv wrapper work, baseline/string
  authority, and Route 8 contraction are no-action as broad targets.
- Accepted follow-up categories are semantic reader migration or row-scoped
  diagnostic/oracle replacement with retained prepared fallback and
  positive/absent/invalid/duplicate-or-conflict/mismatch/fallback proof.

Route constraints:

| Route | Constraint inventory |
| --- | --- |
| Route 1 producer/constant | One selected publication producer read moved earlier, but scalar materialization, producer/constant, ALU, memory/call operand, publication, printer, and oracle consumers remain. Homes, storage, registers, rematerialization, move records, and final records stay retained. |
| Route 2 select-chain/direct-global | One select-chain publication path moved earlier, but direct-global/select-chain helpers remain fallback/oracle for publication, call, memory, ALU, FP, producer, edge-copy, and diagnostics. Materialization sequence and context policy stay retained. |
| Route 3 memory/source | Selected memory/source identity readers and one oracle row are proven only under route/prepared agreement. Address formation, frame/global/TLS relocation, offsets, addressing legality, source-home policy, materialization, final operands, x86 wrapper behavior, and target policy stay retained. |
| Route 4 publication | Selected publication-source and block-entry row evidence is agreement-gated. Publication mechanics, move/home/storage policy, immediate/output spelling, wrapper compatibility, block-order emission, printer/debug, and oracle surfaces remain retained. |
| Route 5 edge/join-source | Selected join-source reader and one oracle row are agreement-gated. Parallel-copy scheduling, source/destination homes, move bundles, execution sites, cycle temporaries, branch policy, wrappers, final edge-copy records, and emitted output remain retained. |
| Route 6 call-use source | Selected AArch64 and one x86 scalar route-debug source boundary are agreement-gated. ABI placement, wrapper kind, clobbers, outgoing stack sizing, byval lanes, variadic counts, helper/carrier protocols, value homes, move bundles, aggregate transport, final call records, storage, movement, printer/debug, and emitted output remain retained. |
| Route 7 comparison/condition | Selected comparison provenance consumer and one oracle row are agreement-gated. Branch targets, suffix mapping, fused legality, hazards, emitted-register state, final order, final assembler rows, branch policy, printer/debug, wrappers, and expected strings remain retained. |
| Route 8 return-chain | Importable only as its own owner/schema line. It owns target-neutral same-block return-chain identity, while return ABI, homes, registers, scratch policy, ALU records, final return emission order, helper/oracle expected strings, and target policy remain outside Route 8. |

### Consumer-Boundary Evidence

| Target | Evidence inventory |
| --- | --- |
| AArch64 | AArch64 remains the primary selected-reader proof target. It can thread narrow route views beside prepared context for selected semantic readers, but `PreparedBirModule`, `PreparedFunctionLookups`, call-plan, address-materialization, move-bundle, and value-home lookup handles remain production/pass-context delivery surfaces. AArch64 also retains target ownership for ABI, frame/stack layout, address formation, value homes, move planning, branch/fused-compare policy, helper protocols, final records, diagnostics, and expected strings. |
| x86 | Proven reuse is limited to Route 6 scalar source agreement through `ConsumedPlans` and one route-debug row. `ConsumedPlans` remains the compatibility boundary; broad x86 call wrapper, memory/publication wrapper, route-debug, frame/register/ABI formatting, instruction-selection, and emission migration are not proven. Prepared fallback remains required for absent, invalid, duplicate/conflict, mismatched, and compatibility-sensitive cases. |
| riscv | Current evidence is wrapper-boundary and no-change evidence only, especially around prepared edge-publication output. No imported riscv route-view migration is proven. Any riscv migration must wait for an AArch64-proven semantic route view or retained-owner boundary and must preserve riscv target-local frame/register/ABI/formatting/emission policy. |

### Missing Or Ambiguous Evidence

- No required Step 1 source was missing.
- Draft 155 supplies retirement intent and reject signals, not executable
  ownership decisions. Later steps must still classify fields, lookup groups,
  wrappers, diagnostics, compatibility adapters, proof criteria, and blockers
  before any retirement or demotion recommendation can be treated as ready.
- The inventory is not yet a classification. Field-by-field
  `PreparedBirModule` ownership, lookup-group ownership, duplicate helper/API
  deletion candidates, and E1-E5 readiness recommendations belong to later
  steps.
