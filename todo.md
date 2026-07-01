Status: Active
Source Idea Path: ideas/open/480_semantic_instruction_result_frame_slot_write_event_producer.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Semantic Write Event Producer Contract

# Current Packet

## Just Finished

Completed Step 2 contract for idea 480 by defining when a semantic
instruction-result frame-slot write/materialization event may be produced.

Contract decision:

- No bounded implementation packet is justified from current prepared/prealloc
  evidence.
- Current rows provide semantic identity and final destination facts for
  `%t23`, but no explicit materialization point where `%t23` is written into
  slot `#21`.
- A producer that derives an event from raw BIR, branch conditions, final
  homes, storage plans, object/slot layout, or `authority=none` moves would be
  unsound for this source idea.

Positive event contract:

| Fact group | Required for `available` semantic write event |
| --- | --- |
| Semantic result identity | Function, producer block/instruction, result value id/name/type, opcode/kind, operands/immediates. |
| Materialization event site | Stable event phase/site and block/instruction coordinate or equivalent prepared event identity. |
| Event source/result | Must match the semantic result `%t23`, not `%t22` or another carrier. |
| Destination identity | Destination value id/name/type, frame slot id, stack object id, stack offset, size, and alignment. |
| Destination coherence | Frame slot/object match by function/object id and layout. |
| Authority | Producer explicitly owns semantic result materialization into the destination frame slot. |

Required statuses / fail-closed cases:

| Case | Disposition |
| --- | --- |
| Missing semantic result identity | `missing_semantic_result_identity`. |
| Raw BIR or branch-condition-only evidence | `missing_materialization_point`. |
| Final home, storage plan, stack object, or frame slot only | `missing_materialization_point`. |
| `%t22 -> %t23` `authority=none` before-instruction move | `storage_only_move`. |
| Event source/result differs from `%t23` | `source_result_mismatch`. |
| Destination differs from slot `#21` / object `#21` / offset `156` / size `4` / align `4` | `destination_mismatch`. |
| Unknown or unsupported authority | `unsupported_authority`. |
| `%t22` select-result stack destination | `unsupported_boundary`; separate owner. |
| `%t1` / `%t7` pointer/provenance rows | `unsupported_boundary`; separate owner. |
| `%t2` / `%t8` unsupported-terminator rows | `unsupported_boundary`; separate owner. |
| Event-authority consumption, interval/source facts, branch-stack-load authority, RV64 | Downstream non-goals. |

Exact blocker:

- Missing explicit semantic instruction-result frame-slot materialization
  point for `%t23 = bir.ne i32 %t22, 0` into slot `#21`.
- The missing producer must record function/site, semantic producer identity,
  result value id/name/type, event source/result `%t23`, destination
  slot/object/offset/size/alignment, and authority before any downstream
  carrier or interval population can be sound.

Artifacts:

- `build/agent_state/480_step2_semantic_write_event_producer_contract/contract.md`

## Suggested Next

Execute Step 3 from `plan.md` as a routing/blocker packet:
`Route Semantic Write Event Producer Blocker`.

Step 3 should record that no implementation is selected from current evidence
and name the missing explicit materialization-point producer surface.

## Watchouts

- Do not implement a collector over final homes/storage/object layout and call
  it a semantic write event.
- Do not reuse `%t22 -> %t23` `authority=none` movement as semantic
  materialization.
- Keep event-authority consumption, interval facts, source facts,
  branch-stack-load authority, and RV64 out of scope.

## Proof

Delegated proof:

```sh
git diff --check
```

Result: passed.
