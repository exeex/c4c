Status: Active
Source Idea Path: ideas/open/192_residual_route_view_consumer_migration_map.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Locate Residual Prepared Consumers

# Current Packet

## Just Finished

Step 2 residual-consumer inventory complete for `plan.md` Step 2. The outline
below distinguishes the selected reader already migrated by ideas 182-188 from
unmigrated prepared readers found through `PreparedBirModule`,
`PreparedFunctionLookups`, route-specific prepared helpers, prepared-printer,
target-wrapper, and backend oracle-test discovery.

| Route | Migrated selected reader | Unmigrated residual prepared readers | Classification | Explicit unknowns / boundary notes |
| --- | --- | --- | --- | --- |
| Route 1 producer/constant | `value_publication_may_read_register_index(...)` route-first publication-source reader from idea 184. | AArch64 scalar producer/materialization reads in `dispatch_producers.cpp`, `alu.cpp`, `comparison.cpp`, `calls.cpp`, and `memory.cpp` still call `find_prepared_same_block_scalar_producer(...)` or `evaluate_prepared_same_block_integer_constant(...)`; prealloc comparison/call/publication planning helpers still use the same producer/constant facts; `prepared_printer/select_chains.cpp` and backend lookup/dispatch tests still compare prepared producer/constant answers. | Production lowering for AArch64 scalar, call, memory, comparison, and publication paths; printer/debug for prepared select-chain dumps; oracle/test for `backend_prepared_lookup_helper`, AArch64 dispatch/scalar-record tests, and prepared stack/frame tests; retained target/prepared policy for value homes, storage, registers, rematerialization spelling, move records, and machine-record formation. | Unknown whether every residual AArch64 producer/constant read can share one route facade; Step 3 should pick one subpath, not claim route-wide producer-helper retirement. Return-chain operand recovery remains separate owner/schema evidence, not Route 1. |
| Route 2 select-chain/direct-global | AArch64 scalar ALU control-publication `select.result` path in `lower_scalar_select_publication(...)` from idea 185. | `find_prepared_direct_global_select_chain_dependency(...)`, `find_prepared_store_source_direct_global_select_chain_dependency(...)`, and `find_prepared_scalar_select_chain_materialization(...)` remain in prealloc call/publication planning, AArch64 `calls.cpp`, `memory.cpp`, and `alu.cpp`; prepared-printer select-chain output and route oracle tests still read prepared select-chain/source-producer facts. | Production lowering for AArch64 call-argument, store-source publication, memory, ALU, and scalar materialization paths; printer/debug for prepared select-chain dump; oracle/test for lookup-helper, store-source publication, scalar-record, and x86 handoff tests; retained target/prepared policy for materialization sequence choice, storage/home selection, memory operand formation, call-publication policy, and final record spelling. | Unknown which residual direct-global readers are pure semantic reads versus coupled to store/call/publication policy until Step 3 selects a concrete candidate. |
| Route 3 memory/source | AArch64 indirect-callee stored-value source consumer from idea 186. | AArch64 `memory.cpp`, `globals.cpp`, `frame_slot_address.cpp`, `fp_value_materialization.cpp`, `dispatch_value_materialization.cpp`, `comparison.cpp`, and x86 `module.cpp` still call `find_prepared_memory_access(...)`, `find_prepared_global_load_access(...)`, `find_prepared_same_block_global_load_access(...)`, or prepared memory lookup maps; prealloc call/publication/variadic planning still uses memory-access helpers; prepared stack-layout/frame/memory tests remain direct oracles. | Production lowering for AArch64 and x86 memory/global/frame/FP/call-source paths; oracle/test for prepared memory operand records, stack-layout, frame-stack call contract, x86 handoff, and lookup-helper tests; retained target/prepared policy for target addressing, offsets, frame/stack objects, TLS/global relocation data, pointer materialization, base-plus-offset legality, volatile/address-space handling, and final memory operand records. | Route 3 has a hard unknown/policy boundary from idea 190: semantic route reads must not absorb prepared target-addressing policy. Residual x86 memory reads are target-wrapper or production lowering depending on call site and need separate candidate scoping. |
| Route 4 publication | AArch64 dispatch-publication reader `current_block_entry_publication_register(...)` from idea 182. | `find_prepared_current_block_entry_publication(...)`, current/block-entry publication collection, and edge-publication lookup surfaces remain in AArch64 `dispatch_publication.cpp`, call/publication planning helpers, prepared-printer value/publication displays, x86 wrappers/debug, and block-entry/publication oracle tests. | Production lowering for AArch64 publication and call-boundary publication paths; printer/debug for prepared/publication dumps and x86 route debug; target-wrapper for x86 `ConsumedPlans`/debug and public prepared-module dump entry points; oracle/test for block-entry publication and lookup-helper suites; retained target/prepared policy for value-home lookup, storage availability, move planning, publication record construction, and block-order emission. | Unknown whether x86/riscv publication wrappers can reuse the AArch64-proven Route 4 facade; no cross-target Route 4 proof exists. |
| Route 5 edge/join-source | AArch64 current-block join-source reader behind `build_current_block_join_prepared_query_routing(...)` from idea 183. | `PreparedEdgePublicationLookups`, `PreparedEdgePublicationSourceProducerLookups`, `find_indexed_prepared_edge_publications(...)`, `find_indexed_prepared_edge_publication_source_producer(...)`, current-block join parallel-copy source facts, and move-bundle lookups remain in AArch64 `dispatch_edge_copies.cpp`, `dispatch_publication.cpp`, `memory.cpp`, `calls.cpp`, `dispatch_producers.cpp`, prealloc publication planning, x86 joined-branch tests, and prepared-printer select-chain/publication output. | Production lowering for AArch64 edge-copy, join-source, call/memory publication-source, and dispatch producer paths; printer/debug for prepared edge/source-producer display; oracle/test for current-block join routing, x86 handoff joined-branch, store-source publication, and lookup-helper suites; retained target/prepared policy for parallel-copy scheduling, out-of-SSA placement, source/destination homes, move-bundle selection, branch spelling, and final edge-copy records. | Unknown if edge-publication source-producer uses in call/memory paths belong under Route 5 migration first or under their Route 2/6/3 caller family; Step 3 should choose one route-owned candidate and name shared fallback behavior. |
| Route 6 call-use source | Direct-global select-chain call-argument materialization consumer from idea 187, plus one x86 `ConsumedPlans` reuse point from idea 189. | `PreparedCallPlanLookups`, `find_prepared_call_argument_source_producer_materialization(...)`, `find_prepared_call_result_late_publication(...)`, call argument/result plan selectors, before-call argument moves, after-call result lane bindings, call-boundary effects, and publication-source routing remain in AArch64 `calls.cpp`, prealloc `call_plans.cpp`, x86 `ConsumedPlans` wrappers, prepared-printer calls output, and backend call-boundary/frame-stack tests. | Production lowering for AArch64 call argument/result/publication lowering; target-wrapper for x86 `ConsumedPlans`, `consume_prepared_function_lookups(...)`, and public x86 prepared APIs; printer/debug for prepared call dump and route debug; oracle/test for call-boundary owner, frame-stack call contract, x86 direct extern call, and lookup-helper suites; retained target/prepared policy for ABI register/stack placement, variadic FPR counts, clobber/preserve sets, byval lanes, outgoing stack sizing, late-publication mechanics, helper resources, carrier protocols, and call-record spelling. | x86 has only the one proven Route 6 scalar argument reuse; riscv has no imported reuse proof. Unknown which call-result/lane readers can be migrated without moving ABI policy. |
| Route 7 comparison/condition | `aarch64::codegen::lower_materialized_compare_condition_branch(...)` from idea 188. | `find_prepared_fused_compare_operand_producer_facts(...)`, `find_prepared_materialized_condition_producer(...)`, prepared scalar producer/select-chain fallback calls, branch-condition helpers, and materialized-compare join helpers remain in AArch64 `comparison.cpp`, prealloc `comparison.cpp`/`control_flow.hpp`, ALU/scalar fallback paths, x86 joined-branch tests, and fused-compare/materialized-condition oracle tests. | Production lowering for AArch64 fused compare, materialized condition, branch operand, and scalar comparison paths; oracle/test for branch-control, x86 handoff joined-branch, and lookup-helper Route 7 suites; retained target/prepared policy for branch spelling, fused legality, condition-code selection, hazard handling, emitted-register state, final branch/compare record ordering, ALU result storage, and return-chain-adjacent recovery. | Return-chain remains explicitly unknown/out of Route 7 scope unless a separate owner/schema imports it. Materialized compare join/render-contract helpers mix comparison provenance with x86 render policy and should not be treated as pure Route 7 migration without more investigation. |

## Suggested Next

Execute Step 3 from `plan.md`: choose one next selected-consumer migration or a
retained-policy reason for each route family using the table above. Start with
one separable candidate per route and name the route facts, fallback behavior,
and proof that would reject a testcase-shaped shortcut.

## Watchouts

- This inventory is not a retirement map. It names residual readers sufficient
  for Step 3 candidate selection, but draft 155 still needs field-by-field
  `PreparedBirModule` and `PreparedFunctionLookups` retirement analysis later.
- Treat ideas 182-188 as selected-consumer evidence, not route-wide completion.
- Treat idea 189 as one x86 Route 6 interface-reuse proof only; riscv has no
  imported reuse proof.
- Printer/debug, oracle/test, target-wrapper, and retained target/prepared
  policy consumers are first-class residuals, not noise.
- Keep target policy, ABI policy, address-materialization policy, final
  emission policy, and return-chain outside Routes 1-7 unless a separate owner
  says otherwise.
- Route 3 needs explicit prepared-policy fallback after idea 190; do not move
  address-materialization or target-addressing policy into route facts.

## Proof

Docs/lifecycle-only packet; no build required. Verification was targeted
source discovery with `rg` plus AST-backed `c4c-clang-tool-ccdb` symbol queries
on `src/backend/prealloc/prepared_lookups.cpp` and function-signature queries
on `src/backend/prealloc/prepared_lookups.hpp`; `todo.md` now contains the
Step 2 route-family residual-consumer table outline with classifications,
migrated readers, residual readers, and explicit unknowns. Existing
`test_after.log` remains the proof log path; no implementation files changed.
