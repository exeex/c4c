# Phase E2 Prepared Lookup API Private Pass-Context Readiness

Status: Step 1 scoped baseline complete.

Source idea:
`ideas/open/223_phase_e2_prepared_lookup_api_private_pass_context_readiness.md`

## Scope

This document is the durable Phase E2 analysis surface for public prepared
lookup API/private pass-context readiness. E2 classifies whether a named public
prepared API boundary or helper family can be drafted for later contraction. It
does not implement contractions, delete helpers, privatize lookup groups,
rename aggregate facades, move target policy into BIR, open draft 155 / E5, or
claim broad prepared retirement.

Step 1 records the factual baseline and candidate list that later steps will
inventory and classify. No candidate below is implementation permission until a
later step names every public consumer, retained prepared fallback or policy
surface, diagnostic/oracle boundary, and proof shape for exactly one public API
boundary or helper family.

## Step 1 Sources Read

| Source | Step 1 facts E2 may rely on |
| --- | --- |
| `ideas/closed/217_phase_e0_prepared_bir_merge_readiness_and_ownership_boundary_audit.md` | E0 accepted the target end state: BIR owns semantics, prepared owns policy, prepared APIs shrink toward private pass context, and duplicate semantic helpers are the deletion target. E0 stayed analysis-only, kept draft 155 / E5 unopened, and rejected selected route readers as broad prepared retirement proof. |
| `docs/bir_prealloc_fusion/phase_e0_prepared_bir_merge_readiness.md` | Field and lookup-group maps classify `PreparedFunctionLookups` as aggregate compatibility/pass-context, with selected semantic subfacts inside some groups but no lookup group deletion, privatization, or aggregate replacement ready. E0 names the E2 candidate set as one-reader adapters or one helper/oracle row at a time. |
| `ideas/closed/218_phase_e1_semantic_duplicate_candidate_triage.md` | E1 triage accepted only two implementation follow-ups, one for control-flow identity helpers and one for route identity helpers, while deferring public prepared API contraction to E2 and broad diagnostics/oracles to E3. |
| `docs/bir_prealloc_fusion/phase_e1_semantic_duplicate_candidate_triage.md` | E1 classified control-flow identity helpers and route source/publication/join/call/comparison identity helpers as ready to draft one implementation idea, but kept aggregate forwarding, target-policy, fallback/oracle, diagnostic/string, and cross-target surfaces blocked or deferred. |
| `ideas/closed/219_phase_e1_control_flow_identity_helper_contraction.md` | The closed E1 control-flow slice proved only the selected `find_prepared_control_flow_branch_target_labels(...)` helper path. It added agreement-gated BIR structured successor identity while preserving prepared behavior for non-agreement paths. It did not retire aggregate `PreparedControlFlow`, delete public prepared APIs, or move branch/output policy into BIR. |
| `ideas/closed/220_phase_e1_route_identity_helper_contraction.md` | The closed E1 route slice proved only the selected Route 7 AArch64 `find_prepared_fused_compare_operand_producer_facts(...)` path consumed by conditional branch lowering. It preserved prepared fallback and output authority and did not open broad Route 1-7 migration, prepared aggregate retirement, wrapper/printer changes, helper-oracle changes, or expected-string rewrites. |
| `docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md` | `PreparedFunctionLookups` remains a seven-group aggregate compatibility and pass-context surface. Route-native candidates are limited to selected Route 3 memory/source, Route 4 publication, Route 5 edge/join-source, Route 6 call-use source, and Route 7 comparison provenance subfacts, each one consumer or adapter boundary at a time with prepared fallback retained. |
| `docs/bir_prealloc_fusion/phase_d2_retained_surface_consumer_switch_analysis.md` | D2 records current production, printer/debug, target-wrapper, oracle, pass-context, aggregate, baseline, string-authority, and return-chain retained boundaries. Selected Route 3-7 adapters are useful evidence, but they did not remove public prepared helper groups, aggregate lookup fields, target wrappers, printer/debug rows, oracle families, or target policy ownership. |
| `src/backend/prealloc/module.hpp` | `PreparedBirModule` is still the prepared phase artifact carrying BIR plus target profile, route, invariants, names, control flow, value locations, stack/frame/addressing/liveness/regalloc state, call/publication/storage/carrier/helper/intrinsic/inline-asm state, phases, and notes. It also exposes public inline prepared helpers over policy-bearing fields. |
| `src/backend/prealloc/prepared_lookups.hpp` | `PreparedFunctionLookups` currently exposes public lookup groups `call_plans`, `address_materializations`, `memory_accesses`, `move_bundles`, `value_homes`, `edge_publications`, and `edge_publication_source_producers`; construction remains public through `make_prepared_function_lookups(...)` and `make_prepared_move_bundle_lookups(...)`. |
| Current accepted baseline and hook state | The accepted baseline evidence from D2/E0/E1 remains a proof guardrail: prior full-suite recovery reached 3428/3428 and E0/E1 lifecycle close proof used matching string-authority guard logs. No Step 1 repo-local hook reminder line was present in `todo.md`; this document therefore records the scope without changing hook-managed review state. |

Missing required inputs: none found during Step 1.

## Step 1 Scoped Baseline

E2 starts after two E1 helper contractions closed, but those closures are
narrow selected-helper evidence only:

- `find_prepared_control_flow_branch_target_labels(...)` proved an
  agreement-gated BIR structured successor identity path for one branch-target
  label helper while preserving prepared behavior for non-agreement cases.
- `find_prepared_fused_compare_operand_producer_facts(...)` proved a selected
  Route 7/prepared agreement path for AArch64 comparison provenance consumed
  by conditional branch lowering while preserving prepared fallback and output
  authority.

These two facts do not prove that any whole lookup group can be deleted, that
aggregate `PreparedFunctionLookups` can be hidden or retired, that
`PreparedBirModule` can be demoted, or that public prepared fallback/oracle
APIs can be privatized. They also do not move target policy into BIR or route
authority. E2 must therefore reason about one public prepared API boundary or
one helper family at a time.

Aggregate lookup construction remains outside E2 implementation scope.
`make_prepared_function_lookups(...)`,
`make_prepared_move_bundle_lookups(...)`, and the aggregate
`PreparedFunctionLookups` delivery shape remain compatibility/pass-context
surfaces until every production, wrapper, printer/debug, oracle, fallback, and
policy consumer for a named sub-surface has a replacement or retained-owner
decision.

Public fallback/oracle APIs remain outside Step 1 contraction. E2 may identify
where a later one-boundary idea could move a selected semantic read behind a
private pass context, but it must preserve prepared authority for absent,
invalid, duplicate/conflict, ambiguous, mismatch, policy-sensitive fallback,
printer/debug, wrapper, helper-oracle, supported-path, baseline, and
expected-string behavior until equivalent proof exists.

E3, E4, Route 8, and E5 remain separate owner lines:

- E3 owns broad prepared diagnostic, printer/debug, route-debug, and
  helper-oracle replacement unless a row is only proof harness for one named
  semantic reader.
- E4 owns cross-target wrapper convergence and wrapper output compatibility.
- Route 8 return-chain work remains a separate owner/schema line and must not
  be folded into Route 1-7 source, comparison, predecessor, or generic route
  facade work.
- Draft 155 / E5 and broad `PreparedBirModule` or `PreparedFunctionLookups`
  retirement remain unopened.

## Step 1 Candidate List

| Candidate surface | Starting semantic evidence | Retained baseline boundary for Step 2 |
| --- | --- | --- |
| Route 6 call-use source adapter paths in `call_plans` | Route 6 can own selected call argument/result source identity under route/prepared agreement; prior D2/E0 evidence includes one scalar x86 compatibility point and AArch64 selected call-use source boundaries. | Call ABI/layout policy, helper/carrier protocols, result lanes, call records, value homes, move bundles, aggregate transport, x86 `ConsumedPlans`, wrappers, printer/debug rows, call oracles, fallback, and expected strings remain prepared/target-owned. |
| Route 3 memory/source adapter paths in `memory_accesses` | Route 3 can own selected memory access/source identity for one reader under route/prepared agreement. | Address formation, frame/global/TLS relocation, offsets, address spaces, volatile handling, materialization, final operands, value-home/source-home policy, target wrappers, memory/address diagnostics, fallback, and oracles remain prepared/target-owned. |
| Route 4/5 publication, edge, and join adapter paths in `edge_publications` and `edge_publication_source_producers` | Route 4 can own selected publication identity and Route 5 can own selected edge/join-source identity under agreement. | Publication record construction, block-order emission, edge-copy mechanics, move/home/storage policy, stack-source extension, wrappers, printer/debug rows, helper-oracle behavior, fallback, and emitted output remain retained. |
| Route 1/2/5/6/7 source-producer helper or printer/oracle rows | E1/D2 evidence names route-native producer/constant, select-chain/direct-global, join-source, call-use source, comparison provenance, and materialized-condition facts as selected semantic candidates. | Materialization sequence policy, direct-global policy when coupled to memory/call/publication contexts, storage/home/move policy, call/publication routing, prepared printer `select_chains`, route-debug rows, helper-oracle strings, fallback, wrappers, and expected strings remain retained. |
| Identity-only `move_bundles` audit candidates | Adjacent Route 5/6 source identity may exist for a future one-reader question. | Move scheduling, out-of-SSA placement, cycle temporaries, source/destination homes, storage, ABI move phases, execution sites, final move records, target rendering, wrappers, move-bundle oracles, and fallback remain prepared/target-owned. |
| Identity-only `value_homes` audit candidates | A future selected value/source identity reader may be separable from policy, but no current group contraction boundary is ready. | Storage authority, register choice, stack slot/offset, rematerialization spelling, decoded-home/formal-publication inputs, operand rendering, value-home policy, wrappers, value-home oracles, and fallback remain prepared/target-owned. |
| Proven E1 helper: `find_prepared_control_flow_branch_target_labels(...)` | One agreement-gated BIR structured successor identity path is proven for the selected helper. | Branch spelling, edge-copy scheduling, block-order emission, move policy, wrapper output, printer/debug rows, helper-oracle behavior, mismatch/non-agreement fallback, expected strings, and aggregate control-flow/public API surfaces remain retained. |
| Proven E1 helper: `find_prepared_fused_compare_operand_producer_facts(...)` | One selected Route 7/prepared agreement path is proven for AArch64 comparison provenance in conditional branch lowering. | Branch targets, suffix mapping, fused legality, hazards, emitted-register state, final instruction order, final assembler rows, printer/debug, helper-oracle behavior, fallback, wrappers, expected strings, and aggregate route/lookup public surfaces remain retained. |

## Step 1 Assumptions For Later Steps

- A later E2 follow-up can be accepted only if it names exactly one public
  prepared API boundary or one helper family.
- Candidate readiness must not be inferred from facade renames, wrapper moves,
  construction reshuffles, baseline refreshes, expected-string rewrites,
  unsupported downgrades, or aggregate field movement.
- Step 2 must inventory production, printer/debug, target-wrapper, oracle,
  fallback, and expected-string consumers for each candidate before Step 3
  assigns readiness buckets.
- If a candidate needs diagnostic/oracle parity before public contraction, it
  defers to E3. If it needs cross-target wrapper migration, it defers to E4.
  If it needs another semantic helper contraction, it defers to a named E1
  follow-up. If it depends on return-chain ownership or broad prepared
  retirement, it is outside E2 and defers to Route 8 or E5 respectively.

## Step 1 Completion Check

- All required source idea, Phase E0, Phase E1, D2, ownership-map, header,
  baseline, and local hook-state inputs were inspected.
- The two closed E1 helpers are recorded as selected-helper proof only, not as
  proof of whole lookup-group deletion, aggregate construction retirement, or
  fallback/oracle privatization.
- Aggregate lookup construction, public fallback/oracle APIs, E3, E4, Route 8,
  E5, and draft 155 are explicitly outside E2 implementation scope.
- The candidate list covers the surfaces named by the source idea and leaves
  consumer inventory and readiness classification for later steps.
