Status: Active
Source Idea Path: ideas/open/99_bir_prealloc_control_publication_lookup_boundary_audit.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Inventory Prealloc Transfer And Publication Plans

# Current Packet

## Just Finished

Step 2 - Inventory Prealloc Transfer And Publication Plans is complete. I used
`c4c-clang-tool-ccdb function-signatures` on
`src/backend/prealloc/out_of_ssa.cpp`,
`src/backend/prealloc/publication_plans.cpp`, and
`src/backend/prealloc/formal_publications.cpp`, plus
`c4c-clang-tool function-signatures` on the corresponding headers and
`src/backend/prealloc/control_flow.hpp`, before reading targeted source ranges.

### Step 1 BIR Control Fact Summary

Step 1 established BIR authority for function/block CFG identity, return/branch
terminators, switch-as-compare-plus-branch lowering, scalar PHI incoming values
and `PhiIncoming::label_id`, typed `bir::Value` identity, compare
`BinaryInst` facts, and explicit `SelectInst` facts. The provisional missing
target-neutral facts were select-chain recognition side facts, compare-expression
side facts, and aggregate PHI copy authority.

### Prealloc Transfer And Publication Classification

| File | Prealloc fact | Classification | Producer evidence | Consumer / publication evidence | Step 1 comparison |
| --- | --- | --- | --- | --- | --- |
| `src/backend/prealloc/control_flow.hpp` | Prepared control-flow publication carriers: `PreparedControlFlowFunction::blocks`, `branch_conditions`, `join_transfers`, and `parallel_copy_bundles`; `PreparedControlFlowBlock` stores terminator kind and target labels; `PreparedBranchCondition` stores materialized-bool or fused-compare branch operands and targets. | `prealloc-transfer-plan-authority` | Structs `PreparedControlFlow`, `PreparedControlFlowFunction`, `PreparedControlFlowBlock`, `PreparedBranchCondition`, `PreparedJoinTransfer`, and `PreparedParallelCopyBundle`. The header comment says shared consumers must take branch semantics from `branch_conditions` and former phi/join obligations from `join_transfers`. | Lookup helpers `find_prepared_control_flow_function`, `find_prepared_branch_condition`, `find_prepared_control_flow_block`, `find_prepared_control_flow_branch_target_labels`, and `published_prepared_compare_join_continuation_targets` expose the prepared plans to consumers. | Legitimate prealloc publication layer over BIR facts, but `PreparedControlFlowBlock` duplicates BIR terminator target facts as a prepared lookup surface. No follow-up yet because this looks like target-facing publication, not new semantic authority. |
| `src/backend/prealloc/out_of_ssa.cpp` | Scalar PHI edge transfers into explicit `PreparedJoinTransfer` and `PreparedEdgeValueTransfer` records. | `prealloc-transfer-plan-authority` | `lower_phi_nodes` walks top-of-block `bir::PhiInst`, creates `PhiLoweringPlan`, records `bir::LocalSlot::phi_observation`, emits `LoadLocalInst`, appends predecessor `StoreLocalInst`, and calls `make_join_transfer`. `make_join_transfer` copies `phi.incomings`, interns `incoming.label_id`/`incoming.label`, and builds one edge transfer per incoming. | `out_of_ssa_module` stores these in `function_control_flow->join_transfers`; `build_parallel_copy_bundles` consumes `join_transfers` into `PreparedParallelCopyBundle` moves and steps. | BIR owns PHI semantic values and incoming labels. Prealloc owns the target-facing transfer plan required after removing PHIs, so this is not a duplicate semantic source. |
| `src/backend/prealloc/out_of_ssa.cpp` | Loop-carried PHI edge classification. | `prealloc-rederives-bir-control-fact` | `classify_phi_join_transfer_kind` calls `block_has_successor` and `is_reachable_without_revisiting`, walking BIR terminators by preferred labels to decide `PreparedJoinTransferKind::LoopCarry` versus `PhiEdge`. | The kind is stored in `PreparedJoinTransfer::kind` by `lower_phi_nodes` and published through `PreparedControlFlowFunction::join_transfers`. | BIR owns CFG and terminator target labels, but does not publish a target-neutral loop-carry/join-transfer classification. Prealloc re-derives CFG reachability to choose a transfer kind; this may be a candidate for Step 5 only if Step 3/4 show consumers depend on the distinction semantically. |
| `src/backend/prealloc/out_of_ssa.cpp` | Parallel-copy execution site, execution block label, move ordering, and cycle-temp steps. | `prealloc-transfer-plan-authority` | `build_parallel_copy_bundles` groups join-transfer edge moves by `(predecessor_label, successor_label)`; `classify_join_transfer_parallel_copy_execution_site` uses prepared predecessor successor count and successor predecessor count; `make_parallel_copy_bundle` orders moves and emits `SaveDestinationToTemp` for cycles. | `PreparedParallelCopyBundle` records `execution_site`, optional `execution_block_label`, `moves`, `steps`, and `has_cycle`; lookup helpers in `control_flow.hpp` include `find_prepared_parallel_copy_bundle`, `published_prepared_parallel_copy_execution_block_label`, and out-of-SSA move lookup helpers. | BIR owns CFG and values. Prealloc owns the concrete parallel-copy scheduling and placement plan; the critical-edge test is a target-facing placement decision derived from prepared CFG counts. |
| `src/backend/prealloc/out_of_ssa.cpp` | Reducible PHI funnel materialization into BIR `SelectInst` and select-materialized join transfers. | `prealloc-rederives-bir-control-fact` | `materialize_phi` and `materialize_funnel_subtree` collect leaf labels, find a compare with `find_compare_for_condition`, walk branch targets, and emit a `bir::SelectInst`; `try_materialize_root_phi_block` records `PreparedJoinTransferCarrierKind::SelectMaterialization`. | `collect_select_materialized_join_transfers` later scans existing first-in-block `bir::SelectInst`, matches it to `PreparedBranchCondition` predicate/compare operands, and publishes select edge transfers. | Step 1 marked select-chain recognition side facts as provisionally missing. This prealloc path reconstructs branch/funnel shape and compare facts rather than consuming a stable BIR "select came from this chain" contract. Keep this as a Step 3 focus item. |
| `src/backend/prealloc/out_of_ssa.cpp` | Branch-owned join-transfer source and compare-join continuation labels. | `prealloc-rederives-bir-control-fact` | `annotate_branch_owned_join_transfers` searches `function_control_flow.branch_conditions`, finds linear predecessors with `find_prepared_linear_join_predecessor`, maps true/false edge-transfer indices, and publishes source branch/incoming labels. `publish_branch_owned_join_transfer_continuation_labels` calls `find_prepared_compare_join_continuation_targets`. | Fields `source_branch_block_label`, `source_true_transfer_index`, `source_false_transfer_index`, `source_true_incoming_label`, `source_false_incoming_label`, `continuation_true_label`, and `continuation_false_label` are published on `PreparedJoinTransfer`; `published_prepared_compare_join_continuation_targets` exposes continuation labels. | BIR owns branch targets, compare instructions, and PHI incoming labels. Prealloc re-associates them into a branch-owned join publication contract. This is probably legitimate publication authority, but the association logic is duplicated shape analysis and should feed Step 3. |
| `src/backend/prealloc/out_of_ssa.cpp` | Short-circuit continuation branch-condition publication. | `prealloc-rederives-bir-control-fact` | `publish_short_circuit_continuation_branch_conditions` matches entry branch targets, finds short-circuit join context and branch plan, locates continuation blocks, then calls `find_trailing_branch_compare` to publish additional fused-compare `PreparedBranchCondition` records. | New `PreparedBranchCondition` entries carry predicate, compare type, lhs/rhs, true/false continuation labels, and `can_fuse_with_branch = true`. | BIR owns compare and branch facts, but prealloc publishes continuation-specific branch conditions not present as a single BIR fact. This should remain a Step 3 select/compare boundary item. |
| `src/backend/prealloc/publication_plans.hpp` and `.cpp` | Scalar current-block entry publication plans from value homes. | `prealloc-transfer-plan-authority` | `PreparedScalarPublicationPlan` stores source value, destination home, hook kind, storage encoding, stack/immediate/pointer details, and current-block entry publication availability. `plan_prepared_scalar_publication` derives these from `PreparedScalarPublicationInputs`, `PreparedValueHome`, and optional `PreparedBlockEntryPublication`. | `prepared_scalar_publication_available`, `prepared_scalar_publication_status_name`, and `prepared_scalar_publication_hook_kind_name` expose availability and target hook shape. | Step 1 BIR owns typed value identity; prealloc legitimately owns storage/home publication facts. No duplicated BIR control semantics here. |
| `src/backend/prealloc/publication_plans.hpp` and `.cpp` | Store-source and store-global publication plans. | `prealloc-transfer-plan-authority` | `PreparedStoreSourcePublicationPlan` records source value, destination access, source home, destination stack/pointer details, source producer, recovered source, direct-global-select-chain flags, and writeback/pending/duplicate flags. `plan_prepared_store_source_publication`, `plan_prepared_store_global_publication`, and `plan_pending_prepared_store_global_publications` populate these from prepared addressing and value-location inputs. | Availability/status/intent helpers and the plan object publish store-local, store-global, and pointer-store writeback transfer details for consumers. | This is prealloc storage/addressing publication authority. It consumes BIR store/load values but does not claim new BIR control authority. |
| `src/backend/prealloc/publication_plans.cpp` | Select-chain dependency and scalar select-chain materialization publication. | `prealloc-rederives-bir-control-fact` | `prepared_select_chain_source_producer`, `prepared_select_chain_contains_direct_global_load`, `find_prepared_direct_global_select_chain_dependency`, and `find_prepared_scalar_select_chain_materialization` trace source-producer chains and select-materialization roots before an instruction index. | `PreparedScalarSelectChainMaterialization`, `PreparedDirectGlobalSelectChainDependency`, and `PreparedStoreSourceDirectGlobalSelectChainDependency` publish `root_is_select`, root instruction index, and direct-global dependency facts into store/scalar publication decisions. | Step 1 explicitly marked select-chain recognition side facts as missing from the stable BIR contract. Prealloc is re-deriving these facts from prepared producer chains, so this is a likely Step 3/4 contract-gap candidate if consumers depend on it. |
| `src/backend/prealloc/publication_plans.cpp` | Recovered narrow store source for wide local load. | `prealloc-transfer-plan-authority` | `find_prepared_recovered_narrow_store_source_for_wide_local_load` uses prepared memory accesses, stack layout, matching load/store ranges, and integer widths to recover a preceding byte store source. | `PreparedRecoveredStoreSourcePublication` and `PreparedStoreSourcePublicationPlan::recovered_source_value` publish the recovered source for store-source planning. | This is memory/addressing recovery, not BIR control duplication. |
| `src/backend/prealloc/formal_publications.hpp` and `.cpp` | Formal parameter publication plans. | `prealloc-transfer-plan-authority` | `PreparedFormalPublicationPlan` records formal index, value name/id, type, ABI source, home, and action. `plan_prepared_formal_publication` checks formal ABI (`passed_on_stack` / `passed_in_register`) and prepared value homes. | `plan_prepared_formal_publications`, `prepared_formal_publication_available`, and status/action names publish incoming register/stack-to-home plans. | BIR owns parameter values and ABI annotations; prealloc legitimately owns the target-facing publication action based on value homes. No control semantic overlap. |
| `src/backend/prealloc/publication_plans.cpp` with `formal_publications.*` | Fixed-formal store-source publication classification. | `prealloc-transfer-plan-authority` | `plan_prepared_fixed_formal_store_source_publication` combines `plan_prepared_store_source_publication` with `plan_prepared_formal_publications`, matching value name/id, home, type, and non-byval formal source. | `PreparedFixedFormalStoreSourcePublication` publishes whether a store source is a fixed formal source and carries both store-source and formal publication plans. | Publication-only cross-plan correlation. It relies on prepared names/homes and formal BIR params, not duplicated BIR control semantics. |

## Suggested Next

Begin Step 3 by tracing select-chain and compare-join classification across
`src/backend/prealloc/out_of_ssa.cpp`, `src/backend/prealloc/control_flow.hpp`,
`src/backend/prealloc/publication_plans.*`, and
`src/backend/prealloc/prepared_lookups.*`. Focus on whether the
`prealloc-rederives-bir-control-fact` rows above should remain publication
authority, become `bir-missing-target-neutral-fact`, or become lookup/printer
contract gaps.

## Watchouts

- This is analysis-only; do not edit implementation files.
- Do not turn the Step 2 `prealloc-rederives-bir-control-fact` rows into
  follow-up ideas yet. Step 3 must first decide whether the re-derived
  select/compare/join associations are consumer-visible semantic contracts or
  only legitimate prepared publication plans.
- The clearest Step 3 probes are `materialize_phi` /
  `materialize_funnel_subtree`, `collect_select_materialized_join_transfers`,
  `annotate_branch_owned_join_transfers`,
  `publish_short_circuit_continuation_branch_conditions`, and
  `find_prepared_scalar_select_chain_materialization`.
- The scalar/store/formal publication plan rows are classified as target-facing
  prealloc transfer/publication authority unless later lookup/printer evidence
  shows consumers must infer BIR control semantics from their shapes.

## Proof

Delegated proof command:

```sh
git diff --quiet -- src/backend/bir src/backend/prealloc && printf 'analysis-only Step 2 proof: no implementation changes under src/backend/bir or src/backend/prealloc\n' > test_after.log
```

Result: passed. `test_after.log` contains:
`analysis-only Step 2 proof: no implementation changes under src/backend/bir or src/backend/prealloc`.
