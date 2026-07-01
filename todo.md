Status: Active
Source Idea Path: ideas/open/480_semantic_instruction_result_frame_slot_write_event_producer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Semantic Write Event Producer Inputs

# Current Packet

## Just Finished

Completed Step 1 audit for idea 480 by classifying where `%t23 = bir.ne i32
%t22, 0` becomes or fails to become a semantic instruction-result frame-slot
write/materialization event for slot `#21`.

Producer input classification:

| Stage / input | Evidence | Verdict |
| --- | --- | --- |
| Semantic result production | Raw BIR in `logic.end.14`: `%t23 = bir.ne i32 %t22, 0`. | Semantic identity exists; no frame-slot write event is attached. |
| Branch consumer | `bir.cond_br i32 %t23, block_5, block_6`. | Consumer context only. |
| Prepared condition | `branch_condition logic.end.14 kind=fused_compare condition=%t23 compare=ne i32 %t22, 0`. | Confirms compare identity; not write/materialization authority. |
| Final value home | `home %t23 value_id=17 kind=stack_slot slot_id=21 offset=156`. | Final placement only; not an event. |
| Storage plan | `storage %t23 frame_slot ... slot#21+stack156 offset=156`. | Destination encoding only; not producer authority. |
| Stack object / frame slot | object `#21`, slot `#21`, offset `156`, size `4`, align `4`. | Destination facts only. |
| Before-instruction move into `%t23` | `move_bundle phase=before_instruction authority=none block_index=11 instruction_index=3`; `move from_value_id=16 to_value_id=17 destination_storage=stack_slot`. | Rejected `storage_only_move`; source is `%t22`, not semantic `%t23`. |
| `%t22` select materialization / publications | join transfer and parallel copies target `%t22`; publications are `unsupported_destination_storage`. | Separate `%t22` stack-destination boundary. |
| Branch-stack-load row | `%t23` row remains `status=missing_policy slot=#21 object=#21`. | Downstream consumer authority, still blocked. |

Protected boundaries:

| Boundary | Disposition |
| --- | --- |
| `%t22` select-result stack destination | Separate select-result/stack-destination owner. |
| `%t1` / `%t7` pointer/provenance rows | Separate pointer/provenance owner. |
| `%t2` / `%t8` unsupported-terminator rows | Separate branch-site/terminator owner. |
| Path/no-clobber interval facts | Later owner after a real event exists. |
| Event-authority consumption, source facts, branch-stack-load authority, RV64 | Downstream non-goals and blocked. |

First Step 2 contract target:

- Define when a semantic instruction-result frame-slot write/materialization
  event may be produced.
- Required candidate fields: function, semantic producer block/instruction,
  result value id/name/type/opcode, event source/result, destination
  slot/object/offset/size/alignment, stable event site, and explicit authority.
- Required fail-closed cases: raw-shape-only, branch-condition-only,
  final-home/storage/object-only, `%t22 -> %t23` `authority=none`
  storage-only movement, source/result mismatch, destination mismatch,
  unsupported authority, and protected boundaries.

Artifacts:

- `build/agent_state/480_step1_semantic_write_event_producer_audit/audit.md`

## Suggested Next

Execute Step 2 from `plan.md`: Define Semantic Write Event Producer Contract.
Decide whether the current producer/prealloc layers can emit a bounded
semantic write/materialization event surface, or whether the route remains
blocked by absence of an explicit materialization point.

## Watchouts

- Do not treat final homes, storage plans, stack objects, frame slots, value
  names, raw BIR adjacency, or dump order as event authority.
- Do not reuse `%t22 -> %t23` `authority=none` movement as semantic
  materialization evidence.
- Do not consume events through event-authority, interval, source-fact,
  branch-stack-load, or RV64 paths in this plan step.

## Proof

Delegated proof:

```sh
git diff --check
```

Result: passed.
