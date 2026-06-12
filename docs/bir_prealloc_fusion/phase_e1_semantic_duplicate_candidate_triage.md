# Phase E1 Semantic Duplicate Candidate Triage

Status: Step 1 evidence inventory complete; Step 2 readiness classification pending.

Source idea: `ideas/open/218_phase_e1_semantic_duplicate_candidate_triage.md`

## Scope

This document is the durable Phase E1 analysis surface for semantic duplicate
candidate triage. E1 starts from Phase E0 evidence and classifies only
duplicate semantic helper families or row-scoped semantic/oracle surfaces. It
does not implement migrations, delete helpers, privatize prepared APIs, move
target policy into BIR, open draft 155 / E5, or claim broad
`PreparedBirModule` or `PreparedFunctionLookups` retirement.

Step 1 records the evidence inventory and skeleton. No readiness bucket is
assigned in Step 1; every candidate family below is explicitly pending Step 2.

## Step 1 Sources Read

| Source | Evidence captured for E1 |
| --- | --- |
| `ideas/closed/217_phase_e0_prepared_bir_merge_readiness_and_ownership_boundary_audit.md` | E0 closure accepted BIR-owns-semantics / prepared-owns-policy as the target end state, named duplicate semantic helpers as the primary future deletion target, preserved draft 155 / E5 as unopened, and required later work to be separately scoped. |
| `docs/bir_prealloc_fusion/phase_e0_prepared_bir_merge_readiness.md` | Main E0 field/group ownership map, E1 semantic duplicate candidate table, code-size-positive helper/API proof requirements, no-reduction surfaces, E3/E4 blockers, and draft 155 / E5 rewrite criteria. |
| `ideas/closed/181_phase_d_mir_consumer_bir_view_switch_plan.md` | Phase D direction: consumer migration proceeds one semantic group at a time; prepared answers remain oracles until equivalence is proven; target/layout/codegen policy stays outside BIR; return-chain requires separate owner/schema work. |
| `ideas/closed/203_phase_d2_retained_surface_consumer_switch_analysis.md` | D2 closure opened one-surface Route 3 through Route 7 follow-ups and rejected broad migration, prepared API deletion, aggregate retirement, draft 155, and expectation weakening. |
| `docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md` | Route 1-7 residual production, printer/debug, target-wrapper, oracle/test, fallback, policy, and proof expectations after selected route-view readers. |
| `docs/bir_prealloc_fusion/phase_d2_retained_surface_consumer_switch_analysis.md` | Retained-surface dependency map and accepted proof shapes for one semantic reader or one diagnostic/oracle row per Route 3 through Route 7 surface. |
| `docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md` | `PreparedFunctionLookups` group map: selected semantic subfacts exist inside some groups, but no group is deletion-, privatization-, or aggregate-replacement-ready. |
| `ideas/closed/207_route3_memory_source_semantic_reader.md` | Route 3 selected memory/source reader can consume route identity only under route/prepared agreement; address formation, relocation, final operands, and target policy remain prepared-owned. |
| `ideas/closed/208_route3_memory_source_oracle_printer_row.md` | Route 3 row-scoped oracle/printer replacement keeps prepared diagnostics authoritative for fallback and non-agreement paths. |
| `ideas/closed/209_route4_publication_source_semantic_reader.md` | Route 4 selected publication-source reader can use route/prepared agreement while publication mechanics, wrapper output, and target policy remain retained. |
| `ideas/closed/210_route4_block_entry_publication_printer_debug_row.md` | Route 4 selected block-entry publication printer/debug row can use route-native attribution only with prepared agreement and byte-stable row text. |
| `ideas/closed/211_route5_current_block_join_source_semantic_reader.md` | Route 5 selected current-block join-source reader can use route/prepared agreement while join, edge, move-bundle, branch, output, and wrapper behavior remain prepared/target-owned. |
| `ideas/closed/212_route5_edge_join_oracle_printer_row.md` | Route 5 row-scoped helper-oracle evidence is diagnostic metadata only; absent, invalid, duplicate/conflict, memory-source, mismatch, unsupported, printer, and wrapper paths remain retained. |
| `ideas/closed/213_route6_call_source_consumer.md` | Route 6 selected call-result source consumer can use route/prepared agreement while ABI, wrappers, aggregate transport, final call records, printer behavior, storage, movement, and emitted behavior remain prepared/target-owned. |
| `ideas/closed/214_route6_x86_scalar_source_route_debug_row.md` | Route 6 x86 scalar source route-debug row is one agreement-gated debug row; `ConsumedPlans` and expected strings remain authoritative and no broad x86 wrapper migration is implied. |
| `ideas/closed/215_route7_comparison_provenance_consumer.md` | Route 7 selected comparison provenance consumer can use route/prepared/BIR agreement while branch behavior, suffix mapping, fused legality, final assembler, and expected strings remain prepared/AArch64-owned. |
| `ideas/closed/216_route7_comparison_oracle_row.md` | Route 7 materialized-condition helper-oracle row can use route-native evidence while preserving prepared oracle assertions, fallback, wrappers, branch-control output, and expected strings. |
| `src/backend/prealloc/module.hpp` | `PreparedBirModule` field inventory and public inline lookup/helper surfaces over the aggregate. |
| `src/backend/prealloc/prepared_lookups.hpp` | `PreparedFunctionLookups` groups and aggregate construction entry points. |
| Current accepted baseline state | Evidence from D2/E0 closure notes: accepted baseline and string authority are proof guardrails; prior accepted full-suite state was restored to 3428/3428, and E0 close proof used matching string-authority logs. No baseline refresh, helper rename, unsupported downgrade, or expected-string rewrite is admissible E1 evidence. |

Missing required inputs: none found during Step 1.

Ambiguous or deferred evidence: E0 intentionally names candidate families, not
implementation-ready slices. Step 2 must still identify exact helper/API
surfaces, public consumers, retained prepared fallback or policy, and proof
shape before any family can move out of pending classification.

## E0 Target End State Captured

| End-state line | Step 1 evidence |
| --- | --- |
| BIR owns semantics | Candidate semantic owners include BIR module/function/block/value identity, CFG facts, route source/publication/join/call/comparison annotations, liveness/use-def annotations, intrinsic metadata facts, and future Route 8 return-chain schema only if separately designed. |
| Prepared owns policy | Target profile, ABI placement, register/stack/layout/address policy, move scheduling, branch spelling, call/helper protocols, final records, wrapper behavior, emitted output, and expected strings remain prepared/target-owned unless a later source names a replacement owner. |
| Prepared APIs become private pass context | Future contraction is possible only for named one-reader adapters or one diagnostic/oracle row after all public production, wrapper, printer/debug, and oracle consumers are accounted for. |
| Duplicate semantic helpers are deletion candidates | Code-size-positive deletion can count only when a helper stops duplicating semantic authority already owned by BIR/route facts; facade renames, wrapper moves, aggregate reshuffles, and target-policy moves do not count. |

## Header Surface Inventory

### `PreparedBirModule`

`PreparedBirModule` contains BIR payload and target/prepared state:
`module`, `target_profile`, `route`, `invariants`, `names`, `control_flow`,
`value_locations`, `stack_layout`, `addressing`, `liveness`,
`register_group_overrides`, `regalloc`, `frame_plan`,
`dynamic_stack_plan`, `call_plans`, `store_source_publications`,
`variadic_entry_plans`, `storage_plans`, `i128_carriers`, `f128_carriers`,
`atomic_operations`, `intrinsic_carriers`, `inline_asm_carriers`,
`f128_runtime_helpers`, `i128_runtime_helpers`, `completed_phases`, and
`notes`.

The header also exposes public inline lookup helpers for register group
overrides, addressing, value locations, frame plans, dynamic stack plans, call
plans, variadic helper operand homes, storage plans, wide carriers, atomics,
intrinsics, inline asm, and runtime helpers. Step 2 must separate pure
semantic duplicate forwarding from retained policy, diagnostics, fallback, and
compatibility delivery.

### `PreparedFunctionLookups`

`PreparedFunctionLookups` contains `call_plans`,
`address_materializations`, `memory_accesses`, `move_bundles`, `value_homes`,
`edge_publications`, and `edge_publication_source_producers`. It is built by
`make_prepared_function_lookups(...)`; move-bundle lookup construction is also
public through `make_prepared_move_bundle_lookups(...)`.

The ownership map classifies the aggregate as retained compatibility and pass
context. Route-native candidates exist only inside selected subfacts, and no
lookup group is Step 1 evidence for deletion, privatization, or aggregate
replacement.

## Step 1 Candidate Family Skeleton

| Candidate family | Candidate semantic authority from E0 | Retained prepared/policy/oracle surfaces from Step 1 | Step 2 status |
| --- | --- | --- | --- |
| aggregate BIR semantic forwarding | Direct `bir::Module` graph, stable BIR function/block/value/link identity, BIR CFG facts, and route annotations. | `PreparedBirModule` wrapper delivery, target-profile threading, phase/pass context, prepared printers/dumps, diagnostics, fallback/oracle calls, and public compatibility helpers. | Pending Step 2 classification. |
| control-flow identity helpers | BIR function/block graph, terminator successors, block labels, route edge/join annotations, and any future separate return-chain schema. | Edge-copy scheduling, branch policy, block-order emission, wrappers, route-debug rows, prepared printer/debug, fallback/oracle checks, and expected strings. | Pending Step 2 classification. |
| route source/publication/join/call/comparison identity helpers | Route 1-7 source, publication, join, call-use, and comparison provenance annotations under route/prepared agreement. | Addressing, value homes, storage, move scheduling, call ABI, publication mechanics, wrapper behavior, printer/debug rows, helper oracles, and failure-mode fallback. | Pending Step 2 classification. |
| liveness identity helpers | BIR use/def, live interval, address-taken, and requires-home annotations where they are pure semantic facts. | Prepared pass ordering, stack-object association, allocation-policy consumers, ambiguity diagnostics, and consumers that combine liveness with target policy. | Pending Step 2 classification. |
| intrinsic metadata adapters | BIR intrinsic family, operation, feature, operand-role, memory, and side-effect facts where those are semantic metadata. | Carrier completeness, required-feature policy, call plans, operand/result homes, helper protocols, target behavior, and missing-fact diagnostics. | Pending Step 2 classification. |
| row-scoped diagnostic/oracle helpers | Route-native or BIR-native facts for one positive row at a time, as proven by Route 3-7 row closures. | Prepared fallback for absent, invalid, duplicate/conflict, mismatch, unsupported, target-policy-sensitive, compatibility-sensitive, printer/debug, wrapper, helper-oracle, and expected-string paths. | Pending Step 2 classification. |

## Baseline And Proof Guardrails

- E1 must not use baseline refreshes, expected-string rewrites, helper renames,
  unsupported downgrades, or timeout masking as evidence.
- Route 3 through Route 7 closure notes prove only bounded selected readers or
  rows; they do not prove broad prepared API contraction.
- Public prepared fallback and diagnostics remain authoritative unless a later
  candidate records equivalent positive, absent, invalid, duplicate/conflict,
  mismatch, fallback, printer/debug, wrapper, and expected-string proof.
- Draft 155 / E5 remains unopened.

## Step 1 Completion Check

- All required inputs listed by the source idea and runbook Step 1 were read.
- The E0 target end state and six E1 candidate families are captured.
- Header-level aggregate and lookup surfaces are inventoried.
- Missing required inputs are recorded as none; readiness ambiguity is recorded
  as pending Step 2 classification rather than implementation permission.
