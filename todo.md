Status: Active
Source Idea Path: ideas/open/479_real_semantic_write_event_authority_producer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Real Event Authority Inputs

# Current Packet

## Just Finished

Completed Step 1 audit for idea 479 by classifying real event-authority inputs
for representative `%t23 = bir.ne i32 %t22, 0` into frame slot `#21`.

Carrier input classification:

| Carrier input | Evidence | Verdict |
| --- | --- | --- |
| Function/block | `prepared.func @f`, raw block `logic.end.14`. | Present. |
| Semantic instruction result | `%t23 = bir.ne i32 %t22, 0`. | Present; usable semantic identity. |
| Prepared branch condition | `branch_condition logic.end.14 kind=fused_compare condition=%t23 compare=ne i32 %t22, 0`. | Present; confirms compare/condition identity. |
| Result value home | `home %t23 value_id=17 kind=stack_slot slot_id=21 offset=156`. | Present; destination/value input. |
| Storage plan | `storage %t23 frame_slot ... slot#21+stack156 offset=156`. | Present, but final storage only; not event authority. |
| Stack object | `object #21 func=f name=%t23 type=i32 size=4 align=4`. | Present; destination object input. |
| Frame slot | `slot #21 object_id=21 func=f offset=156 size=4 align=4`. | Present; destination slot input. |
| Event-site evidence | Raw `%t23` instruction and nearby before-instruction bundle coordinates are visible. | Coordinates only; no semantic write authority. |
| Move into `%t23` | `move_bundle phase=before_instruction authority=none block_index=11 instruction_index=3`; `move from_value_id=16 to_value_id=17 destination_storage=stack_slot reason=consumer_stack_to_stack`. | `storage_only_move`; rejected as semantic materialization. |
| Publications / parallel copies | Visible records target `%t22`, not `%t23`, and `%t22` block-entry publications are `unsupported_destination_storage`. | Cannot populate `%t23` event authority; protected `%t22` boundary. |
| Branch-stack-load authority row | `%t23` row remains `status=missing_policy slot=#21 object=#21`. | Downstream only; does not prove event authority. |
| Idea 478 carrier surface | Explicit synthetic event authority can become `available`. | Surface is sufficient; real producer input is missing. |

Protected boundaries:

| Boundary | Disposition |
| --- | --- |
| `%t22` select-result stack destination | Separate select-result / stack-destination owner. |
| `%t1` / `%t7` pointer/provenance rows | Separate pointer/provenance owner. |
| `%t2` / `%t8` unsupported-terminator rows | Separate branch-site/terminator relationship owner. |
| Path/no-clobber interval facts | Later interval owner after real event authority exists. |
| Source-fact, branch-stack-load authority, RV64 | Downstream consumers remain blocked. |

First missing producer fact:

- A durable real semantic write/materialization event authority record for
  `%t23 = bir.ne i32 %t22, 0` into frame slot `#21`.
- The record must prove semantic result identity, event site/source,
  destination slot/object/offset/size/alignment, and authority that this is
  semantic compare-result materialization rather than storage movement.

Artifacts:

- `build/agent_state/479_step1_real_event_authority_audit/audit.md`

## Suggested Next

Execute Step 2 from `plan.md`: Define Real Event Authority Contract. The
contract should decide whether a bounded producer packet can publish real
semantic write-event authority from durable prepared producer evidence, or
whether this route is blocked by absence of an explicit event source.

## Watchouts

- Do not reuse `%t22 -> %t23` `authority=none` stack movement as semantic
  materialization evidence.
- Do not infer event authority from raw BIR adjacency, stack homes, storage,
  offsets, object ids, names, testcase shape, or dump order.
- Keep path/no-clobber interval proof and downstream source-fact,
  branch-stack-load authority, and RV64 consumers out of Step 2/3 unless a
  later packet explicitly owns them.

## Proof

Delegated proof:

```sh
git diff --check
```

Result: passed.
