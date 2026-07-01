Status: Active
Source Idea Path: ideas/open/480_semantic_instruction_result_frame_slot_write_event_producer.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 4 residual disposition for idea 480 after the semantic
write-event producer route was blocked.

Residual decision:

- Idea 480 is close-ready as a routed blocker.
- Current prepared/prealloc evidence identifies `%t23` and its final frame
  slot, but does not expose a materialization point where `%t23` is known to be
  written into slot `#21`.
- No implementation packet should be selected from current evidence.

Representative residuals:

| Item | Evidence | Disposition |
| --- | --- | --- |
| Semantic result identity | `%t23 = bir.ne i32 %t22, 0`; branch condition confirms `condition=%t23 compare=ne i32 %t22, 0`. | Present. |
| Final destination | value id `17`, home slot `#21`, storage `slot#21+stack156`, object `#21`, offset `156`, size `4`, align `4`. | Present, but placement only. |
| Explicit materialization point | No durable event site/source proves `%t23` was written into slot `#21`. | First missing owner. |
| `%t22 -> %t23` movement | `move_bundle phase=before_instruction authority=none`; `from_value_id=16 to_value_id=17`. | Rejected `storage_only_move`; wrong source and no authority. |
| Raw BIR / branch condition / final home / storage / object / slot | Visible. | Insufficient; must not infer event production. |
| `%t22` select-result stack destination | Select materialization and publications target `%t22`. | Protected separate owner. |
| `%t1` / `%t7` pointer/provenance rows | Pointer/provenance gap family. | Protected separate owner. |
| `%t2` / `%t8` unsupported-terminator rows | Branch-site relationship gap family. | Protected separate owner. |
| Event-authority consumption | Requires explicit event production first. | Downstream blocked. |
| Path/no-clobber interval, source facts, branch-stack-load authority, RV64 | Require explicit event plus later facts. | Downstream blocked. |

First remaining owner:

- An explicit semantic result frame-slot materialization-point producer.
- It must record function/event site, semantic producer identity, result value
  id/name/type, event source/result matching `%t23`, destination
  slot/object/offset/size/alignment, and authority proving `%t23` was
  materialized into slot `#21`.
- It must not reconstruct this event from raw BIR adjacency, final homes,
  storage plans, stack object layout, value names, function names, testcase
  shape, dump order, or `authority=none` storage movement.

Lifecycle recommendation:

- Plan-owner should close/split idea 480 as a routed blocker.
- If continuing, activate a new source idea for the explicit semantic result
  frame-slot materialization-point producer.
- Keep idea 480 active only if plan-owner identifies one exact in-scope packet
  based on a newly found explicit materialization point.

Artifacts:

- `build/agent_state/480_step4_semantic_write_event_producer_residual/disposition.md`

## Suggested Next

Plan-owner should decide lifecycle closure/split for idea 480. The next
technical owner, if activated, should be the explicit semantic result
frame-slot materialization-point producer.

## Watchouts

- Do not treat `%t22 -> %t23` `authority=none` movement as semantic
  materialization.
- Do not infer event production from placement, storage, object layout, value
  names, raw shape, testcase shape, or dump order.
- Event-authority, interval, source-fact, branch-stack-load, and RV64 work
  remains blocked until an explicit materialization point exists.

## Proof

Delegated proof:

```sh
git diff --check
```

Result: passed.
