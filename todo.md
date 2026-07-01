Status: Active
Source Idea Path: ideas/open/479_real_semantic_write_event_authority_producer.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route Real Event Authority Producer

# Current Packet

## Just Finished

Completed Step 3 for idea 479 as a routing/blocker packet. No implementation
is selected from current prepared evidence.

Blocker decision:

- The idea 478 carrier API is sufficient.
- Current prepared evidence cannot populate real `%t23` semantic write-event
  authority without forbidden inference.
- The only concrete movement into `%t23` remains `%t22 -> %t23`
  `authority=none` storage movement, which is explicitly rejected as semantic
  compare-result materialization.

Rejected evidence:

| Evidence | Disposition |
| --- | --- |
| Raw `%t23 = bir.ne i32 %t22, 0` in `logic.end.14` | Semantic identity only; not event authority. |
| Prepared `branch_condition ... condition=%t23 compare=ne i32 %t22, 0` | Confirms compare identity only. |
| `home %t23 value_id=17 kind=stack_slot slot_id=21 offset=156` | Final value home; not write/materialization authority. |
| `storage %t23 frame_slot ... slot#21+stack156` | Storage plan only. |
| Object `#21` and slot `#21` layout | Destination layout only. |
| `move_bundle phase=before_instruction authority=none ... move from_value_id=16 to_value_id=17` | `storage_only_move`; rejected. |
| `%t22` block-entry publications / parallel copies | Select-result stack-destination boundary; not `%t23` authority. |

Required lower-level producer surface:

- A durable semantic instruction-result frame-slot write/materialization event
  producer.
- It must explicitly record function/event site, semantic producer identity
  `%t23 = bir.ne i32 %t22, 0`, result value id/name/type, event source/result
  matching `%t23`, destination slot/object/offset/size/alignment, and authority
  proving semantic result materialization into slot `#21`.
- It must not derive authority from raw BIR adjacency, final homes, storage
  layout, object ids, value names, function names, testcase shape, dump order,
  or `authority=none` storage moves.

Preserved boundaries:

| Boundary | Status |
| --- | --- |
| `%t22` select-result stack destination | Separate owner. |
| `%t1` / `%t7` pointer/provenance rows | Separate owner. |
| `%t2` / `%t8` unsupported-terminator rows | Separate owner. |
| Path/no-clobber interval facts | Later owner after real event authority exists. |
| Source-fact population, branch-stack-load authority, RV64 | Downstream and blocked. |

Artifacts:

- `build/agent_state/479_step3_real_event_authority_blocker/blocker.md`

## Suggested Next

Execute Step 4 from `plan.md`: Residual Disposition And Close Readiness.
Classify whether idea 479 should close/split because the first owner is the
lower-level semantic write/materialization event producer, or remain active
only if a precise in-scope producer packet is identified.

## Watchouts

- Do not reopen implementation from current evidence by treating placement or
  storage movement as event authority.
- Any future positive path needs a real producer-side event source, not a
  collector over current final homes or move bundles.
- Downstream interval/source-fact/branch-stack-load/RV64 work remains blocked.

## Proof

Delegated proof:

```sh
git diff --check
```

Result: passed.
