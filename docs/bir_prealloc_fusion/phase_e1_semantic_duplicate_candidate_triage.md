# Phase E1 Semantic Duplicate Candidate Triage

Status: Step 4 deferral and no-action recording complete.

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

## Step 2 Candidate Family Classification

Readiness buckets use the runbook's exact owner names. A `ready to draft one
implementation idea` decision means the next idea must still select one helper
family or one reader; it is not permission to delete an aggregate, remove
fallback, privatize public prepared APIs, or change expected strings.

| Candidate family | Exact helper/API surface | Semantic owner | Retained prepared surface | Public consumers still present | Proof shape before code deletion or helper contraction | Readiness bucket |
| --- | --- | --- | --- | --- | --- | --- |
| aggregate BIR semantic forwarding | `PreparedBirModule`, `prepare_semantic_bir_module_with_options(...)`, `prepare_bir_module_with_options(...)`, `BirPreAlloc::prepared()`, `BirPreAlloc::run()`, and aggregate inline forwarding helpers over `module`, `control_flow`, `names`, and route facts in `src/backend/prealloc/module.hpp`. | Direct `bir::Module` functions, blocks, typed instructions, terminators, values, calls, memory addresses, comparison facts, stable BIR identity annotations, and route annotations. | `PreparedBirModule` wrapper delivery, target-profile threading, pass ordering, prepared printer/dump entry points, diagnostics, fallback/oracle calls, and public compatibility helpers for policy-bearing fields. | Preparation entry points, AArch64 traversal and pass context, x86/riscv wrapper handoff paths, prepared printers/dumps, helper-oracle tests, and any public caller that still receives the prepared aggregate as the compatibility artifact. | Not a deletion proof shape in E1. A later slice must first name one forwarding helper or one direct consumer, prove it reads BIR identity directly, preserve byte-stable printer/oracle output, and show no hidden target-policy dependency. Aggregate removal, facade renaming, and construction reshuffling do not count. | blocked because the surface is target/prepared policy, fallback/oracle, or aggregate compatibility |
| control-flow identity helpers | `PreparedControlFlow`, `PreparedControlFlowFunction`, `PreparedControlFlowBlock`, `resolve_prepared_block_label_id(...)`, `find_prepared_control_flow_function(...)`, `find_prepared_control_flow_block(...)`, `find_prepared_linear_join_predecessor(...)`, `find_prepared_control_flow_branch_target_labels(...)`, and selected branch/join facade helpers in `src/backend/prealloc/control_flow.hpp`. | BIR function/block graph, block labels, terminator successor facts, target-neutral predecessor/successor identity, selected Route 4 publication and Route 5 edge/join annotations, plus future Route 8 return-chain schema only if separately imported. | Edge-copy scheduling, branch policy, block-order emission, join transfer records, move-bundle interaction, route-debug rows, prepared printer/debug rows, fallback/oracle checks, wrappers, and expected strings. | AArch64 edge/publication/control-flow lowering, prepared control-flow and value-location printers, current-block join routing tests, x86 joined-branch/handoff tests, lookup-helper oracles, and wrapper/debug consumers. | Pick one identity-only helper or reader. Prove BIR/route and prepared agreement for positive, absent, invalid, duplicate/conflict, and mismatch cases; keep branch spelling, edge-copy scheduling, move policy, printer/debug rows, wrapper output, and expected strings unchanged. | ready to draft one implementation idea |
| route source/publication/join/call/comparison identity helpers | Route 1/2 helpers `find_prepared_same_block_scalar_producer(...)`, `evaluate_prepared_same_block_integer_constant(...)`, `find_prepared_direct_global_select_chain_dependency(...)`, `find_prepared_store_source_direct_global_select_chain_dependency(...)`; Route 3 helpers `PreparedMemoryAccessLookups`, `find_prepared_memory_access(...)`, `find_prepared_global_load_access(...)`, `find_prepared_same_block_global_load_access(...)`; Route 4/5 helpers `PreparedEdgePublicationLookups`, `PreparedEdgePublicationSourceProducerLookups`, `find_prepared_current_block_entry_publication(...)`, `find_indexed_prepared_edge_publications(...)`, `find_indexed_prepared_edge_publication_source_producer(...)`; Route 6 helpers `PreparedCallPlanLookups`, `find_prepared_call_argument_source_producer_materialization(...)`, `find_prepared_call_result_late_publication(...)`; Route 7 helpers `find_prepared_fused_compare_operand_producer_facts(...)` and `find_prepared_materialized_condition_producer(...)`. | Route 1-7 source, producer, select-chain, direct-global, memory/source, publication, edge/join, call-use, fused-compare, and materialized-condition provenance annotations under route/prepared agreement. | Address formation, value homes, storage, move scheduling, call ABI, publication construction, branch spelling, fused legality, wrapper behavior, printer/debug rows, helper oracles, failure-mode fallback, and all target-policy-sensitive output. | AArch64 production lowering for scalar, memory, publication, call, edge, and comparison paths; x86 `ConsumedPlans` and wrapper/debug paths; riscv prepared emission; prepared printers; route-debug rows; lookup-helper and backend oracle tests. | One route family and one consumer or row at a time. Prove route/prepared agreement plus absent, invalid, ambiguous/conflict, mismatch, policy-sensitive fallback, nearby same-feature coverage, unchanged printer/debug/oracle strings, and no public fallback/oracle consumer left behind for that helper family. | ready to draft one implementation idea |
| liveness identity helpers | `PreparedLiveness`, `PreparedLivenessFunction`, `PreparedLivenessBlock`, `PreparedLivenessValue`, local `find_liveness_function(...)` consumers, and liveness-derived `requires_home_slot`, live-interval, use-point, def-point, and call-point reads in `src/backend/prealloc/liveness.hpp`, `liveness.cpp`, `regalloc.cpp`, `call_plans.cpp`, `i128_runtime_helpers.cpp`, and `f128_runtime_helpers.cpp`. | BIR value use/def, live interval, address-taken, stack-object identity, call-point, and requires-home annotations when they are pure semantic facts. | Liveness pass ordering, stack-object association, allocation pressure, register class/placement decisions, spill/reload policy, ABI/helper policy consumers, ambiguity diagnostics, and consumers that combine liveness with target allocation policy. | Regalloc construction, stack layout/regalloc helpers, call-plan construction, I128/F128 runtime helper planning, prepared printer diagnostics, and any tests that assert allocation or stack-object behavior through liveness-derived state. | First perform a consumer audit that separates pure identity reads from allocation-policy reads. A later proof must show equivalent BIR annotation reads, preserved stack-object ambiguity diagnostics, no allocation-policy behavior removed, and unchanged regalloc/call/helper outcomes for nearby same-feature cases. | needs more analysis before implementation |
| intrinsic metadata adapters | `PreparedIntrinsicCarriers`, `PreparedIntrinsicCarrierFunction`, `PreparedIntrinsicCarrier`, `find_prepared_intrinsic_carriers(...)`, `populate_intrinsic_carriers(...)`, intrinsic printer rows, and metadata/validation adapters in `src/backend/prealloc/intrinsics.cpp` and `special_carriers.hpp`. | BIR intrinsic family, operation, required feature, operand-role, memory operand, memory-access, side-effect, signedness, barrier, vector-shape, and result/operand semantic metadata. | Carrier completeness, required-feature policy, prepared call plans, value homes, operand/result homes, helper protocols, target lowering, missing-fact diagnostics, and intrinsic printer/oracle expected strings. | Prepared intrinsic carrier population and printer output, AArch64 intrinsic/call-plan paths, x86/riscv intrinsic lowering boundaries, call-plan and value-home consumers, helper-oracle tests, and diagnostics for unsupported or missing facts. | First identify one metadata-only adapter that does not consume carrier completeness or target/helper policy. A later proof must show that consumer reads BIR intrinsic metadata directly while carrier completeness, feature policy, helper calls, operand/result homes, missing-fact diagnostics, and expected strings remain prepared-owned. | needs more analysis before implementation |
| row-scoped diagnostic/oracle helpers | Prepared invariant/note rows plus Route 3-7 row surfaces: memory/source helper-oracle or addressing printer rows, block-entry publication printer/debug rows, edge/join helper-oracle or prepared printer rows, x86 Route 6 scalar source route-debug rows, and Route 7 fused-compare or materialized-condition helper-oracle rows. | Route-native or BIR-native facts for one byte-stable positive row at a time, after the corresponding semantic reader or route fact is already proven. | Prepared fallback for absent, invalid, duplicate/conflict, mismatch, unsupported, target-policy-sensitive, compatibility-sensitive, printer/debug, wrapper, helper-oracle, and expected-string paths. | Prepared printer/debug output, route-debug summaries, `backend_prepared_lookup_helper` tests, current-block join/publication/fused-compare/materialized-condition oracles, x86/riscv wrapper-output tests, CLI/dump snippets, and string-authority baselines. | One row at a time. Prove byte-stable positive row output plus identical absent, invalid, duplicate/conflict, mismatch, unsupported, fallback, wrapper, helper-oracle, and expected-string behavior; keep prepared authority for every non-agreement path. This belongs after or beside a named semantic proof, not as broad printer/debug cleanup. | defer to E3 for diagnostic/oracle replacement |

## Step 2 Classification Notes

- `ready to draft one implementation idea` is assigned only where current
  evidence names a real semantic owner, a retained prepared owner, public
  consumers, and a one-helper or one-reader proof shape.
- Aggregate `PreparedBirModule` forwarding is blocked as an aggregate
  compatibility and pass-context surface even though selected forwarding
  helpers inside it may later become evidence for a narrower idea.
- Liveness and intrinsic metadata are plausible semantic duplicate families,
  but current evidence has not yet separated one implementation-ready consumer
  from allocation, carrier, helper, or target policy.
- Row-scoped diagnostic/oracle work is evidence-backed and important, but the
  readiness owner is E3 unless the row is only the proof harness for a named E1
  semantic reader.

## Step 3 Accepted Follow-Up Ideas

Step 3 drafts follow-up ideas only for the Step 2 families classified as
`ready to draft one implementation idea`. These accepted ideas are deliberately
bounded to one semantic duplicate helper family each; each later runbook must
still select one helper or one reader before implementation.

| Accepted follow-up | Ready family | Why ready |
| --- | --- | --- |
| `ideas/open/219_phase_e1_control_flow_identity_helper_contraction.md` | control-flow identity helpers | Ready because Step 2 names direct BIR/route identity ownership for function/block graph, block labels, terminator successors, predecessor/successor identity, and selected edge/join annotations while separately retaining branch policy, scheduling, diagnostics, fallback/oracle behavior, wrappers, printer/debug rows, and expected strings. The idea requires one identity-only helper or reader plus positive, absent, invalid, duplicate/conflict, mismatch, policy-fallback, and output-stability proof before any prepared helper contraction. |
| `ideas/open/220_phase_e1_route_identity_helper_contraction.md` | route source/publication/join/call/comparison identity helpers | Ready because Step 2 names route/prepared agreement as the semantic owner for selected Route 1 through Route 7 source, producer, publication, join, call-use, fused-compare, and materialized-condition identity facts while retaining address formation, value homes, storage, move scheduling, call ABI, publication construction, branch spelling, fused legality, wrappers, printer/debug rows, helper oracles, fallback, target policy, and expected strings. The idea requires one route family and one selected consumer or identity helper plus positive, absent, invalid, ambiguous/conflict, mismatch, policy-fallback, output-stability, and nearby same-feature proof. |

No accepted follow-up is drafted for aggregate BIR semantic forwarding,
liveness identity helpers, intrinsic metadata adapters, row-scoped
diagnostic/oracle helpers, E2, E3, E4, Route 8, broad prepared retirement, or
draft 155 / E5. Aggregate forwarding remains blocked as compatibility and pass
context; liveness and intrinsic metadata still need narrower consumer analysis;
row-scoped diagnostic/oracle replacement remains E3-owned unless a row is only
the proof harness for a named E1 semantic reader. Draft 155 / E5 remains
unopened.

## Step 4 Deferrals And No-Action Decisions

Step 4 separates the two Step 3 accepted follow-ups from surfaces that E1 must
defer, block, or leave untouched. The only ready E1 follow-ups are the
control-flow identity helper contraction idea and the Route 1 through Route 7
identity helper contraction idea. Every other surface below is outside the
current E1 implementation-ready set.

### Ready Follow-Ups

| Surface | Step 4 decision |
| --- | --- |
| control-flow identity helpers | Ready follow-up already drafted as `ideas/open/219_phase_e1_control_flow_identity_helper_contraction.md`; later execution must still choose one identity-only helper or reader and preserve prepared policy, diagnostics, wrappers, fallback/oracle behavior, and strings. |
| route source/publication/join/call/comparison identity helpers | Ready follow-up already drafted as `ideas/open/220_phase_e1_route_identity_helper_contraction.md`; later execution must still choose one route family plus one selected consumer or identity helper and preserve target/prepared policy, diagnostics, wrappers, fallback/oracle behavior, and strings. |

### Explicit Deferrals

| Deferred surface | Deferral owner | Step 4 decision |
| --- | --- | --- |
| liveness identity helpers | E2 or later consumer-audit work | Deferred because current evidence has not separated one pure semantic liveness reader from allocation pressure, stack-object association, ABI/helper policy, regalloc, or call/helper outcomes. |
| intrinsic metadata adapters | E2 or later metadata consumer-audit work | Deferred because current evidence has not isolated one metadata-only adapter from carrier completeness, feature policy, call plans, operand/result homes, helper protocols, target lowering, or missing-fact diagnostics. |
| row-scoped diagnostic/oracle helpers | E3 | Deferred to E3 unless a row is only the proof harness for a named E1 semantic reader; broad printer/debug, helper-oracle, expected-string, and diagnostic replacement is not an E1 action. |
| target policy movement into BIR | E4 or later target-policy design | Deferred because target profile, ABI placement, register/stack/layout/address policy, move scheduling, branch spelling, helper protocols, wrapper behavior, and emitted output remain prepared/target-owned. |
| Route 8 return-chain schema and consumers | Route 8-specific design | Deferred because E1 has no imported return-chain owner/schema proof; future Route 8 facts must be opened separately before any helper contraction can depend on them. |
| draft 155 / E5 prepared rewrite or broad prepared retirement | E5 only after separate rewrite criteria are met | Deferred and unopened; E1 does not open draft 155 / E5, does not claim broad `PreparedBirModule` or `PreparedFunctionLookups` retirement, and does not treat accepted E1 helper ideas as aggregate replacement. |

### Blockers

| Blocked surface | Blocking reason |
| --- | --- |
| aggregate `PreparedBirModule` semantic forwarding | Blocked as aggregate compatibility and pass context. Selected forwarding helpers may become later narrow evidence, but aggregate deletion, facade renaming, public entry-point reshuffling, and prepared aggregate replacement are not code-size-positive E1 proof. |
| aggregate `PreparedFunctionLookups` replacement | Blocked because lookup groups still combine selected route-native subfacts with prepared compatibility, public construction, fallback/oracle behavior, and target/prepared policy. No lookup group is deletion-, privatization-, or aggregate-replacement-ready in E1. |
| broad prepared API privatization | Blocked until every public production, wrapper, printer/debug, fallback/oracle, and compatibility consumer for a named helper family is accounted for by a separate implementation idea and proof. |

### No-Action Decisions

| Surface | No-action decision |
| --- | --- |
| aggregate surfaces | No action in E1 on aggregate deletion, aggregate replacement, broad prepared retirement, public aggregate entry points, aggregate facade naming, or construction reshuffling. These do not count as semantic duplicate helper contraction. |
| target-policy surfaces | No action in E1 on target profile, ABI placement, register/stack/layout/address policy, move scheduling, branch spelling, helper protocols, wrapper behavior, final records, emitted output, or expected target-specific behavior. |
| fallback/oracle surfaces | No action in E1 on prepared fallback authority, helper-oracle assertions, absent/invalid/duplicate/conflict/mismatch paths, unsupported-path behavior, baseline authority, or expected-string authority except as preserved proof around one named semantic helper. |
| diagnostic/string surfaces | No action in E1 on broad printer/debug replacement, route-debug row migration, diagnostic string edits, expected-string rewrites, baseline refreshes, helper renames, unsupported downgrades, timeout masking, or row text changes. |
| cross-target compatibility surfaces | No action in E1 on x86/riscv wrapper output, cross-target handoff behavior, compatibility delivery, CLI/dump snippets, or public prepared artifacts that still bridge target-specific lowering. |

Draft 155 / E5 remains unopened.

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
