Status: Active
Source Idea Path: ideas/open/479_real_semantic_write_event_authority_producer.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 4 residual disposition for idea 479 after the real
event-authority producer route was blocked.

Residual decision:

- Idea 479 is close-ready as a routed blocker.
- The idea 478 carrier API is sufficient and is not the remaining blocker.
- Real `%t23 = bir.ne i32 %t22, 0` still lacks a durable producer-side event
  source proving semantic instruction-result materialization into frame slot
  `#21`.
- No implementation packet should be selected from current prepared evidence.

Representative residuals:

| Item | Evidence | Disposition |
| --- | --- | --- |
| Real `%t23` semantic identity | `%t23 = bir.ne i32 %t22, 0`; prepared branch condition confirms `condition=%t23 compare=ne i32 %t22, 0`. | Present. |
| Real `%t23` destination | value id `17`, slot `#21`, object `#21`, offset `156`, size `4`, align `4`. | Present. |
| Real semantic write/materialization authority | No durable event source exists. | First missing lower-level producer. |
| `%t22 -> %t23` movement | `move_bundle ... authority=none`; `move from_value_id=16 to_value_id=17`. | Rejected `storage_only_move`; not semantic event authority. |
| Raw BIR, final home, storage, object, slot layout | Visible. | Insufficient; must not infer authority. |
| `%t22` select-result stack destination | Publications/parallel copies target `%t22`. | Protected separate owner. |
| `%t1` / `%t7` pointer/provenance rows | Pointer/provenance gap family. | Protected separate owner. |
| `%t2` / `%t8` unsupported-terminator rows | Branch-site relationship gap family. | Protected separate owner. |
| Path/no-clobber interval proof | Requires real event carrier first. | Downstream blocked. |
| Source-fact population, branch-stack-load authority, RV64 | Require real event authority and later interval facts. | Downstream blocked. |

First remaining owner:

- A lower-level semantic instruction-result frame-slot write/materialization
  event producer.
- It must explicitly record function/event site, semantic producer identity,
  result value id/name/type, event source/result matching `%t23`, destination
  slot/object/offset/size/alignment, and authority proving semantic result
  materialization into slot `#21`.
- It must not reconstruct authority from final homes, storage plans, raw BIR
  adjacency, object ids, value names, function names, testcase shape, dump
  order, or `authority=none` storage moves.

Lifecycle recommendation:

- Plan-owner should close/split idea 479 as a routed blocker.
- If continuing, activate a new source idea for the lower-level event producer
  surface.
- Keep idea 479 active only if plan-owner identifies one exact in-scope packet
  based on a newly found explicit producer event source.

Artifacts:

- `build/agent_state/479_step4_real_event_authority_residual/disposition.md`

## Suggested Next

Plan-owner should decide lifecycle closure/split for idea 479. The next
technical owner, if activated, should be the lower-level semantic
instruction-result frame-slot write/materialization event producer.

## Watchouts

- Do not treat `%t22 -> %t23` `authority=none` movement as semantic
  materialization.
- Do not infer event authority from placement, storage, object layout, value
  names, raw shape, testcase shape, or dump order.
- Downstream interval/source-fact/branch-stack-load/RV64 work remains blocked
  until a real event producer exists and later interval facts are proven.

## Proof

Delegated proof:

```sh
git diff --check
```

Result: passed.
