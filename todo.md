Status: Active
Source Idea Path: ideas/open/359_bir_prepared_object_consumer_contract_completion.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit and Design the Shared Prepared Consumer Contract

# Current Packet

## Just Finished

Step 1 audit/design only. Existing BIR/prepared state already represents
pieces of the consumer contract, but targets still need one shared prepared
object-consumer surface that turns those facts into an ordered traversal,
transfer/query plan, and precise diagnostics.

| Contract area | Represented today | Missing contract piece | Likely shared shape |
| --- | --- | --- | --- |
| Block-entry / edge-copy execution sites | `PreparedMovePhase::BlockEntry`, `PreparedMoveBundle::source_parallel_copy_predecessor_label`, `source_parallel_copy_successor_label`, `PreparedParallelCopyBundle::execution_site`, and block-entry publication helpers identify edge-owned copies and destination facts. | No target-independent object-consumer schedule says when a `BlockEntry` bundle is emitted relative to label/body/terminator, nor does it normalize predecessor-terminator vs successor-entry placement for all targets. | `PreparedObjectTraversal` or `for_each_prepared_block_event` yielding `Label`, `BlockEntryCopies`, `Instruction`, `PreTerminatorCopies`, `Terminator` events with attached edge/publication facts. |
| Select / join-transfer carrier classification | `PreparedJoinTransfer`, `PreparedJoinTransferCarrierKind`, `effective_prepared_join_transfer_carrier_kind`, select-chain source-producer lookups, and liveness tests distinguish select-materialized join authority from ordinary values. | Object consumers do not have a single query that answers "emit this select" vs "consume this as a join-transfer carrier"; target code can still rediscover by scanning `join_transfers` or producers. | `classify_prepared_select_consumption(function, block, inst)` returning `RealSelect`, `JoinTransferCarrier`, or precise unsupported status plus related `PreparedJoinTransfer`/edge-copy facts. |
| Value-home transfer planning | `PreparedValueHomeKind` covers `Register`, `StackSlot`, `RematerializableImmediate`, and `PointerBasePlusOffset`; `PreparedScalarPublicationPlan`, decoded-home storage helpers, and edge-copy facts expose many source/destination details. | There is no generic home-to-home transfer plan for object consumers; support is fragmented across scalar publication, store-source publication, decoded storage, and target-local materialization. | `plan_prepared_value_home_transfer(source_home, destination_home, value_type, context)` returning hook intents such as register move, stack load/store, rematerialize immediate, base-plus-offset address materialization, or explicit unsupported status. |
| Frame ownership | `PreparedStackLayout`, `PreparedFramePlanFunction`, frame slots, stack objects, saved-register slots, storage-plan values, and stack-layout tests publish frame size/alignment and owned slots. | Consumers can still need to scan value homes to discover stack-homed publications or temporary needs; there is no object-consumer assertion that every stack home required by publications has prepared frame ownership. | `PreparedObjectFrameView` / `validate_prepared_frame_ownership(function)` exposing frame size, slot lookup, saved-register slots, dynamic-stack facts, and diagnostics for any stack home without a prepared slot. |
| Diagnostic taxonomy | Several local status enums already exist: block-entry publication, edge-copy source facts, scalar publication, typed stack source publication, decoded-home storage, and handoff boundary errors. | These statuses are not unified into object-consumer diagnostics, so target emitters can still collapse missing contract pieces into coarse prepared-module-shape failures. | `PreparedObjectDiagnostic` with stable codes for missing block-entry publication, unsupported move op, unsupported stack width, missing value home, unsupported select carrier, globals/data-section out of scope, and adapters from existing status enums. |
| Shared traversal shape | Prepared control-flow functions publish block order/labels/terminators, move-bundle lookups publish phase/index placement, and target backends have partial prepared consumers. | No shared traversal owns the canonical sequence `label -> block-entry copies -> instructions -> pre-terminator/edge copies -> terminator`; RV64/AArch64 can still assemble local loops and heuristics. | `PreparedObjectFunctionTraversal` constructed from `PreparedFunctionLookups`, yielding stable per-block events and requiring targets to provide emission hooks only for concrete labels, moves, loads/stores, branches, and diagnostics. |

## Suggested Next

Execute Step 2 by adding focused shared-contract tests first. Start with tests
for the traversal schedule and block-entry/edge-copy placement because those
define where the other contract pieces plug in.

## Watchouts

- This idea is about shared BIR/prepared contract completion, not reducing the
  full RV64 gcc torture scan failure count.
- Do not implement globals/data sections, vararg/FPR ABI, or full RV64
  instruction coverage here.
- Do not accept the retracted `20000113-1.c` slice as target-local progress.
- Keep target backends hook-shaped: concrete machine emission only, no
  rediscovery of CFG/data-flow semantics.
- The likely implementation center is shared `prealloc`/prepared consumer
  query code. RV64 `object_emission.cpp` should only consume the resulting
  hooks after the shared tests and helpers exist.

## Proof

Not run. Audit/design-only packet; no implementation or tests changed, and no
`test_after.log` was produced.
