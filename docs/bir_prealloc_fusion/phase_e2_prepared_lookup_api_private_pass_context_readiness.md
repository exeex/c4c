# Phase E2 Prepared Lookup API Private Pass-Context Readiness

Status: Step 2 consumer inventory complete.

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

## Step 2 Consumer Inventory

Step 2 inventories current public consumers only. It deliberately does not
classify readiness, accept follow-up work, or propose implementation edits;
those decisions belong to Step 3.

| Candidate surface | Production consumers | Printer/debug consumers | Target-wrapper consumers | Oracle, fallback, and expected-string consumers | Aggregate/pass-context and retained prepared responsibility notes |
| --- | --- | --- | --- | --- | --- |
| Route 6 call-use source adapter paths in `call_plans` | AArch64 call lowering still reaches public prepared call-plan lookups through `find_indexed_prepared_call_plan(...)` and `require_prepared_call_plan(...)` (`src/backend/mir/aarch64/codegen/calls.cpp:83`, `src/backend/mir/aarch64/codegen/calls.cpp:111`). Call-boundary lowering classifies prepared moves, uses value-home lookups, reads immediate call arguments, and publishes stack-preserved call values through call-plan lookup state (`src/backend/mir/aarch64/codegen/calls.cpp:4096`, `src/backend/mir/aarch64/codegen/calls.cpp:4174`, `src/backend/mir/aarch64/codegen/calls.cpp:9253`). Route 6 source-producer adapters exist beside prepared fallback for scalar call arguments (`src/backend/mir/aarch64/codegen/calls.cpp:6419`, `src/backend/mir/aarch64/codegen/calls.cpp:6468`). | Prepared function summaries and call-plan printer sections still emit callsites, wrapper kinds, argument rows, variadic counts, aggregate transport, and late publications (`src/backend/prealloc/prepared_printer/functions.cpp:418`, `src/backend/prealloc/prepared_printer/calls.cpp:452`). x86 route-debug rows also consume prepared call plans and names for summary/trace output (`src/backend/mir/x86/debug/debug.cpp:60`, `src/backend/mir/x86/debug/debug.cpp:428`). | x86 `ConsumedPlans` stores prepared call plans, `PreparedFunctionLookups`, and the Route 6 index side by side; the accepted x86 scalar source path is agreement-gated and not a wrapper migration (`src/backend/mir/x86/x86.hpp:14`, `src/backend/mir/x86/x86.hpp:95`). | Call-boundary and route-debug tests assert prepared call-plan contracts and exact debug strings, including helper/carrier, wrapper, source-shape, byval, preservation, dynamic-stack, and `ConsumedPlans` behavior (`tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp:3634`, `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp:6327`, `tests/backend/bir/backend_x86_route_debug_test.cpp:1408`). | AArch64 traversal constructs `PreparedFunctionLookups` and separate call-plan lookup handles for pass context (`src/backend/mir/aarch64/codegen/traversal.cpp:74`, `src/backend/mir/aarch64/codegen/traversal.cpp:76`). Prepared retains ABI placement, wrapper kind, clobbers, outgoing stack sizing, byval lanes, variadic FPR counts, helper/carrier protocols, value homes, move bundles, publication routing, aggregate transport, final call records, storage, movement, fallback, and emitted output. |
| Route 3 memory/source adapter paths in `memory_accesses` | AArch64 memory, ALU, calls, and globals still read prepared memory/address state through `find_prepared_memory_access(...)`, `memory_accesses`, address-materialization lookups, and load-local source fallback (`src/backend/mir/aarch64/codegen/memory.cpp:2805`, `src/backend/mir/aarch64/codegen/alu.cpp:1039`, `src/backend/mir/aarch64/codegen/calls.cpp:8401`, `src/backend/mir/aarch64/codegen/globals.cpp:157`). The indirect-callee path can use Route 3 source identity but falls back to prepared same-block load-local source lookup (`src/backend/mir/aarch64/codegen/calls.cpp:8370`, `src/backend/mir/aarch64/codegen/calls.cpp:8448`). | Prepared addressing printer rows remain the visible debug surface for memory accesses and address materializations (`src/backend/prealloc/prepared_printer/addressing.cpp:49`, `src/backend/prealloc/prepared_printer/addressing.cpp:57`). | x86 local-slot/global guards still use prepared memory access helpers for handoff compatibility (`src/backend/mir/x86/module/module.cpp:5453`). | Prepared lookup-helper and AArch64 prepared memory operand tests assert indexed memory-access, address-materialization, absent/duplicate/mismatch, and target-addressing fallback behavior (`tests/backend/bir/backend_prepared_lookup_helper_test.cpp:5256`, `tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp:2123`). Prepared dump snippets remain expected-string consumers (`tests/backend/bir/CMakeLists.txt:497`). | `PreparedFunctionLookups::memory_accesses` and `address_materializations` are pass-context projections built during traversal (`src/backend/prealloc/prepared_lookups.hpp:15`, `src/backend/mir/aarch64/codegen/traversal.cpp:78`). Prepared retains address formation, frame/global/TLS relocation, offsets, address spaces, volatile and base-plus-offset legality, materialization, final operands, source-home policy, target-addressing fallback, and memory/address diagnostics. |
| Route 4/5 publication, edge, and join adapter paths in `edge_publications` and `edge_publication_source_producers` | AArch64 call-boundary and indirect-callee lowering read `edge_publication_source_producers` with local fallback construction when the aggregate is absent (`src/backend/mir/aarch64/codegen/calls.cpp:6695`, `src/backend/mir/aarch64/codegen/calls.cpp:8290`, `src/backend/mir/aarch64/codegen/calls.cpp:8616`). Memory lowering reads `edge_publications` for stack-source publication and source producers for store-source identity (`src/backend/mir/aarch64/codegen/memory.cpp:1970`, `src/backend/mir/aarch64/codegen/memory.cpp:2795`, `src/backend/mir/aarch64/codegen/memory.cpp:3605`). Route 5 current-block join routing consumes prepared edge-publication/value-home fallback beside Route 5 validation (`src/backend/mir/aarch64/codegen/dispatch_producers.cpp:405`, `src/backend/mir/aarch64/codegen/dispatch_producers.cpp:446`). | Prepared value-location and control-flow printers emit block-entry publication, edge publication, source-producer, join transfer, and parallel-copy rows (`src/backend/prealloc/prepared_printer/value_locations.cpp:177`, `src/backend/prealloc/prepared_printer/value_locations.cpp:202`, `src/backend/prealloc/prepared_printer/control_flow.cpp:151`, `src/backend/prealloc/prepared_printer/control_flow.cpp:217`). | x86 and riscv prepared publication paths still consume prepared module or lookup-derived publication state (`src/backend/mir/x86/prepared/dispatch.cpp:56`, `src/backend/mir/riscv/codegen/emit.cpp:320`, `src/backend/mir/riscv/codegen/emit.cpp:531`). | Helper tests and target tests assert current-block publication, join-source, edge-publication, source-producer, move/source status, wrapper output, and joined-branch expected strings (`tests/backend/bir/backend_prepared_lookup_helper_test.cpp:1195`, `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:4200`, `tests/backend/mir/backend_aarch64_current_block_join_routing_test.cpp:44`, `tests/backend/bir/backend_x86_handoff_boundary_joined_branch_test.cpp:11329`). | `PreparedFunctionLookups` exposes both publication groups publicly (`src/backend/prealloc/prepared_lookups.hpp:18`, `src/backend/prealloc/prepared_lookups.hpp:19`). Prepared retains publication record construction, block-order emission, edge-copy mechanics, move/home/storage policy, stack-source extension, immediate payload spelling, duplicate/ambiguous/wrong-reference fallback, wrappers, printer/debug rows, helper-oracle behavior, and emitted output. |
| Route 1/2/5/6/7 source-producer helper or printer/oracle rows | AArch64 call lowering uses Route 1 producer facts through Route 6 adapters only when agreement holds, then falls back to prepared source-producer materialization (`src/backend/mir/aarch64/codegen/calls.cpp:6420`, `src/backend/mir/aarch64/codegen/calls.cpp:6468`). Indirect-callee lowering uses prepared source producers and direct-global select-chain dependency fallback (`src/backend/mir/aarch64/codegen/calls.cpp:8277`, `src/backend/mir/aarch64/codegen/calls.cpp:8349`). Route 7 comparison lowering converts route producer kinds to prepared producer facts and verifies agreement before replacing selected facts (`src/backend/mir/aarch64/codegen/comparison.cpp:241`, `src/backend/mir/aarch64/codegen/comparison.cpp:356`). | Prepared `select_chains`, source-producer, comparison, and route-debug rows remain visible printer/debug surfaces unless a future row proves parity (`src/backend/prealloc/prepared_printer/value_locations.cpp:116`, `src/backend/prealloc/prepared_printer/calls.cpp:372`, `src/backend/mir/x86/debug/debug.cpp:428`). | x86 Route 6 debug and wrapper compatibility consume prepared names/call plans through `ConsumedPlans`; riscv prepared emission still consumes prepared lookup-derived publication state (`src/backend/mir/x86/x86.hpp:95`, `src/backend/mir/riscv/codegen/emit.cpp:537`). | Helper-oracle tests assert producer, direct-global select-chain, call-use source, join-source, comparison provenance, materialized-condition, and fail-closed strings (`tests/backend/bir/backend_prepared_lookup_helper_test.cpp:3297`, `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:8555`, `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:9643`, `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:9797`). Route-debug and branch-control tests assert exact expected strings (`tests/backend/bir/backend_x86_route_debug_test.cpp:1408`, `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp:1459`). | These rows are not one aggregate lookup group. Prepared retains materialization sequence policy, direct-global policy when coupled to memory/call/publication contexts, storage/home/move policy, call/publication routing, route-debug formatting, helper-oracle names/statuses, fallback, wrappers, and expected strings. |
| Identity-only `move_bundles` audit candidates | AArch64 call lowering uses move-bundle lookups for before-call moves, immediate bindings, carrier moves, after-call movement, and stack-preserved publication (`src/backend/mir/aarch64/codegen/calls.cpp:4090`, `src/backend/mir/aarch64/codegen/calls.cpp:5040`, `src/backend/mir/aarch64/codegen/calls.cpp:5128`, `src/backend/mir/aarch64/codegen/calls.cpp:6248`). Other lowering paths iterate prepared move bundles for target record construction (`src/backend/mir/aarch64/codegen/cast_ops.cpp:1405`). | Prepared control-flow/value-location printers expose move bundles, join transfers, and parallel-copy rows (`src/backend/prealloc/prepared_printer/control_flow.cpp:217`, `src/backend/prealloc/prepared_printer/value_locations.cpp:116`). | x86 joined-branch and call-boundary handoff tests mutate/consume move bundles as part of wrapper compatibility (`tests/backend/bir/backend_x86_handoff_boundary_joined_branch_test.cpp:1999`, `tests/backend/bir/backend_x86_handoff_boundary_joined_branch_test.cpp:4881`). | Prepared lookup-helper tests assert move-bundle indexing, move/source relationships, fallback, unsupported move, duplicate/conflict, and edge/join interactions (`tests/backend/bir/backend_prepared_lookup_helper_test.cpp:2521`, `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:4939`). | `PreparedFunctionLookups::move_bundles` and `FunctionLoweringContext::move_bundle_lookups` are pass-context surfaces (`src/backend/prealloc/prepared_lookups.hpp:16`, `src/backend/mir/aarch64/module/module.hpp:79`). Prepared retains move scheduling, out-of-SSA placement, cycle temporaries, source/destination homes, storage, ABI move phases, execution sites, final move records, target rendering, wrappers, move-bundle oracles, and fallback. |
| Identity-only `value_homes` audit candidates | AArch64 call, select-materialization, formal-publication, decoded-home, and memory tests still use prepared value-home lookups for source/destination homes and register/stack decisions (`src/backend/mir/aarch64/codegen/calls.cpp:4096`, `src/backend/mir/aarch64/codegen/select_materialization.cpp:332`, `src/backend/prealloc/formal_publications.cpp:14`, `src/backend/prealloc/decoded_home_storage.cpp:194`). | Prepared value-location printer rows are the public debug surface for value homes and decoded locations (`src/backend/prealloc/prepared_printer/value_locations.cpp:116`, `src/backend/prealloc/prepared_printer/value_locations.cpp:177`). | x86 handoff tests and AArch64 memory operand tests consume value homes as part of target-wrapper and target-record compatibility (`tests/backend/bir/backend_x86_handoff_boundary_joined_branch_test.cpp:1934`, `tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp:2328`). | Prepared lookup-helper tests assert indexed value-home lookup, source/destination homes, move-publication relationships, and fallback behavior (`tests/backend/bir/backend_prepared_lookup_helper_test.cpp:2264`, `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:2622`, `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:3230`). | `PreparedFunctionLookups::value_homes` and `FunctionLoweringContext::value_home_lookups` are public pass-context projections (`src/backend/prealloc/prepared_lookups.hpp:17`, `src/backend/mir/aarch64/module/module.hpp:80`). Prepared retains storage authority, register choice, stack slot/offset, rematerialization spelling, decoded-home/formal-publication inputs, operand rendering, value-home policy, wrappers, value-home oracles, and fallback. |
| Proven E1 helper `find_prepared_control_flow_branch_target_labels(...)` | The public helper remains defined in `control_flow.hpp` and is still called by helper wrappers and tests (`src/backend/prealloc/control_flow.hpp:472`, `src/backend/prealloc/control_flow.hpp:492`, `src/backend/prealloc/control_flow.hpp:599`). E1 proved only one agreement-gated BIR structured successor identity path for the selected helper. | Prepared control-flow printer/debug rows still own branch, join-transfer, and parallel-copy output (`src/backend/prealloc/prepared_printer/control_flow.cpp:151`, `src/backend/prealloc/prepared_printer/control_flow.cpp:217`). | x86 module lowering and joined-branch wrapper tests still consume prepared control-flow branch/handoff behavior (`src/backend/mir/x86/module/module.cpp:719`, `tests/backend/bir/backend_x86_handoff_boundary_joined_branch_test.cpp:11329`). | Helper-oracle tests cover positive, absent, invalid-id, mismatch, and fallback behavior for the branch-target helper (`tests/backend/bir/backend_prepared_lookup_helper_test.cpp:2114`, `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:2132`, `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:2166`). Expected branch/wrapper strings remain protected by target tests. | This helper is not a `PreparedFunctionLookups` field, but it is a retained public prepared API boundary. Prepared retains branch spelling, edge-copy scheduling, block-order emission, move policy, wrapper output, printer/debug rows, helper-oracle behavior, mismatch/non-agreement fallback, expected strings, and aggregate control-flow/public API surfaces. |
| Proven E1 helper `find_prepared_fused_compare_operand_producer_facts(...)` | AArch64 comparison lowering still reads prepared fused-compare branch facts and converts/compares Route 7 facts against prepared operand producer facts before using the selected agreement path (`src/backend/mir/aarch64/codegen/comparison.cpp:201`, `src/backend/mir/aarch64/codegen/comparison.cpp:262`, `src/backend/mir/aarch64/codegen/comparison.cpp:356`). Branch lowering still owns targets, suffixes, fused legality, hazards, and final assembler order outside the helper (`src/backend/mir/aarch64/codegen/comparison.cpp:2106`, `src/backend/mir/aarch64/codegen/comparison.cpp:2439`). | Machine-printer and branch-control debug surfaces still expose fused-compare and branch-condition output (`tests/backend/mir/backend_aarch64_machine_printer_test.cpp:1633`, `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp:1459`). | No broad target-wrapper migration is implied; AArch64 branch policy remains target-owned, while x86/riscv wrappers remain outside this helper. | Prepared helper-oracle and branch-control tests cover fused, materialized-condition, absent-route, invalid-reference, duplicate, mismatch, unfused fallback, and expected-string behavior (`tests/backend/bir/backend_prepared_lookup_helper_test.cpp:7086`, `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:9643`, `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:9797`, `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp:1554`, `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp:1641`). | This helper is a selected public prepared API boundary, not a lookup-group retirement. Prepared/AArch64 retains branch targets, suffix mapping, fused legality, hazards, emitted-register state, final instruction order, final assembler rows, printer/debug, helper-oracle behavior, fallback, wrappers, expected strings, and aggregate route/lookup public surfaces. |

## Step 2 Completion Check

- Every Step 1 candidate has a consumer inventory row with production,
  printer/debug, target-wrapper, oracle/fallback/expected-string,
  aggregate/pass-context, and retained prepared responsibility notes called out
  where applicable.
- Consumer inventory remains separate from readiness classification; no row
  claims deletion, privatization, aggregate hiding, wrapper migration, oracle
  replacement, or expected-string permission.
- The retained public surfaces include both aggregate/pass-context delivery
  (`PreparedFunctionLookups` and AArch64 lookup handles) and the two proven E1
  helpers as separate public prepared API boundaries.

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
