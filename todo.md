Status: Active
Source Idea Path: ideas/open/459_rv64_select_edge_suppression_placement_consumer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Suppression Placement Consumer Evidence

# Current Packet

## Just Finished

Completed Step 1 audit for idea 459. The producer/prepared
`predecessor_edge_consumed_suppression` metadata from idea 458 is coherent, but
RV64 does not yet have a target-consumable suppression row.

Bucket table:

| Bucket | Placement record fields | Edge identity | Source producer | Select carrier | Bundle site | Moves | Current RV64 event visibility | First missing consumer fact |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Coherent suppression-consumer candidate | `kind=predecessor_edge_consumed_suppression`, `status=available`, false-by-default placement record from idea 458 collector. | `logic.rhs.end.13 -> logic.end.14`, `incoming=%t18`, `destination=%t22`. | `%t18 = bir.ule ptr %t15, %t17`. | `%t22 = bir.select ... i32 %t18, 0`. | `before_instruction`, `authority=none`, `block_index=4`, `instruction_index=2`. | Register-destination moves into `%t18`/`t0`, including `consumer_register_to_register` and `consumer_stack_to_register`. | Not visible/consumable today: RV64 does not collect placement records, and traversal only emits before-instruction copy events for bundles with stack-slot destinations. | Expose this exact register-destination bundle as a traversal event and match it to an explicit available placement record before suppressing it. |
| Existing cast-dependency suppression | Dependency-operand authority, not select-edge placement metadata. | Cast-dependency edge only. | Cast producer, not the binary select-edge source producer. | Not the `%t22` route. | `before_instruction`, `authority=none`. | Single stack-destination move. | Already handled by the cast-dependency stack publication guard. | Not reusable for the target register-destination suppression bundle. |
| Existing edge source-producer materialization | No suppression record. | Requires source-parallel-copy predecessor/successor labels. | Select-edge source producer emitted as edge-owned materialization. | Existing edge publication path. | Edge/block-entry publication, not the join before-instruction bundle. | Producer materialization moves. | Separate existing route. | Cannot justify suppressing the target join bundle. |
| Fail-closed adjacent shapes | Missing record, wrong kind/status, mismatched source producer/select carrier/edge/site, unsupported moves, stale stack-load authority, raw inferred shape. | Missing or mismatched edge tuple. | Non-binary, missing, mismatched, or raw-only producer. | Non-select or mismatched carrier. | Wrong phase/block/instruction/authority. | Generic or unrelated moves. | Must remain rejected. | No inference from raw BIR, value ids, block indexes, instruction indexes, filenames, function names, or one dump. |

Artifact:
`build/agent_state/459_step1_suppression_placement_consumer_audit/audit.md`.

## Suggested Next

Step 2: Define RV64 Suppression Consumer Contract.

Specify the exact RV64 contract for consuming
`predecessor_edge_consumed_suppression`: placement-record lookup keys, event
visibility for before-instruction register-destination bundles, move identity
matching, accepted suppression behavior, fail-closed adjacent shapes, owned
files/tests for Step 3, and the Step 3 proof command.

## Watchouts

- Consume only explicit `predecessor_edge_consumed_suppression` placement
  metadata.
- RV64 currently has two separate consumer gaps: it does not collect placement
  records, and prepared traversal does not expose the target register-destination
  before-instruction bundle.
- Do not infer suppression from value ids, block indexes, instruction indexes,
  raw BIR shape, filenames, function names, or one prepared dump.
- Do not add generic stack-to-register or register-to-register move support.
- Do not consume `load_from_stack_slot missing_stack_freshness`.
- Do not reopen idea 456 cast-dependency consumption.
- Keep generic stack-home branch consumer work routed to
  `ideas/open/451_stack_home_branch_operand_materialization.md`.
- Keep pointer-value provenance and generic instruction-side lowering out of
  this plan.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, baseline logs, or
  `review/`.

## Proof

Lifecycle activation validation:

```sh
git diff --check
```

Result: passed for Step 1 audit.
