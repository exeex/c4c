Status: Active
Source Idea Path: ideas/open/480_semantic_instruction_result_frame_slot_write_event_producer.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route Semantic Write Event Producer

# Current Packet

## Just Finished

Completed Step 3 for idea 480 as a routing/blocker packet. No implementation
is selected from current prepared/prealloc evidence.

Blocker decision:

- Current evidence cannot produce a sound semantic instruction-result
  frame-slot write/materialization event for `%t23 = bir.ne i32 %t22, 0` into
  slot `#21`.
- The missing input is an explicit materialization point where semantic result
  `%t23` is known to be written to that frame slot.
- A collector over raw BIR, branch conditions, final homes, storage plans,
  object/slot layout, or `authority=none` movement would violate the source
  idea.

Rejected current evidence:

| Evidence | Rejection reason |
| --- | --- |
| Raw `%t23 = bir.ne i32 %t22, 0` | Semantic identity only; no frame-slot write event. |
| Prepared `branch_condition ... condition=%t23 compare=ne i32 %t22, 0` | Compare identity only. |
| `home %t23 value_id=17 kind=stack_slot slot_id=21 offset=156` | Final home only. |
| `storage %t23 frame_slot ... slot#21+stack156` | Storage encoding only. |
| Object `#21` / slot `#21` layout | Destination facts only. |
| `%t22 -> %t23` before-instruction move with `authority=none` | `storage_only_move`; wrong source and no authority. |
| `%t22` select materialization / publications | Protected select-result stack-destination boundary. |

Required explicit surface:

- A prepared semantic instruction-result frame-slot materialization event
  producer.
- For `%t23`, it must record function/site, semantic producer identity,
  result value id/name/type, event source/result matching `%t23`, destination
  slot/object/offset/size/alignment, and authority that this event materializes
  `%t23` into slot `#21`.

Preserved boundaries:

| Boundary | Status |
| --- | --- |
| `%t22` select-result stack destination | Separate owner. |
| `%t1` / `%t7` pointer/provenance rows | Separate owner. |
| `%t2` / `%t8` unsupported-terminator rows | Separate owner. |
| Event-authority consumption through ideas 478/479 | Downstream and blocked. |
| Path/no-clobber interval facts | Downstream and blocked. |
| Source facts, branch-stack-load authority, RV64 | Downstream non-goals and blocked. |

Artifacts:

- `build/agent_state/480_step3_semantic_write_event_producer_blocker/blocker.md`

## Suggested Next

Execute Step 4 from `plan.md`: Residual Disposition And Close Readiness.
Classify whether idea 480 should close/split as a routed blocker, or remain
active only if plan-owner identifies a new exact in-scope packet based on an
explicit materialization point.

## Watchouts

- Do not reopen implementation by treating final placement or storage movement
  as materialization authority.
- A future positive path needs a real producer-side materialization event, not
  inference from prepared dumps.
- Event-authority, interval, source-fact, branch-stack-load, and RV64 work
  remains blocked.

## Proof

Delegated proof:

```sh
git diff --check
```

Result: passed.
