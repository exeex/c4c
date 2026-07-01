Status: Active
Source Idea Path: ideas/open/479_real_semantic_write_event_authority_producer.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Real Event Authority Contract

# Current Packet

## Just Finished

Completed Step 2 contract for idea 479 by defining the real semantic
instruction-result frame-slot write/materialization event authority contract.

Contract decision:

- The idea 478 carrier/status surface is sufficient.
- Current prepared evidence is insufficient to populate real `%t23` event
  authority.
- No bounded real producer implementation packet is justified from the current
  rows, because the only visible concrete movement into `%t23` is
  `authority=none` storage movement from `%t22`.

Accepted real authority facts:

| Fact group | Required for `available` real event authority |
| --- | --- |
| Semantic result identity | Function, producer block/instruction, result value id/name/type, opcode/kind, operand identities, and immediates. |
| Event identity | Event kind, event site/phase, block/instruction coordinate or equivalent stable site, and event source/result identity. |
| Destination identity | Destination value id/name/type, frame slot id, stack object id, stack offset, size, and alignment. |
| Destination relationship | Frame slot and stack object must match by function/object id and offset/size/alignment. |
| Event authority | Explicit authority that the event writes/materializes the semantic instruction result into the destination frame slot/object. |
| Source/result coherence | Event source/result must be `%t23`, not `%t22`, a storage carrier, or a predecessor/consumer value. |

Rejected / fail-closed cases:

| Shape | Disposition |
| --- | --- |
| Raw `%t23` BIR instruction adjacency | `missing_event_authority`; raw shape is not an event carrier. |
| Final home, storage plan, stack object, frame slot only | `missing_event_authority`; placement is not a write event. |
| `%t22 -> %t23` `authority=none` before-instruction move | `storage_only_move`; not semantic compare-result materialization. |
| Block-entry publications / parallel copies targeting `%t22` | Protected select-result stack-destination boundary. |
| Event source/result mismatch | `semantic_result_mismatch`. |
| Destination slot/object/offset/size/alignment mismatch | `destination_mismatch`. |
| Unknown or unsupported event authority | `missing_event_authority` / `unsupported_event_authority`. |
| `%t1` / `%t7` pointer/provenance rows | Protected separate owner. |
| `%t2` / `%t8` unsupported-terminator rows | Protected separate owner. |
| Path/no-clobber interval, source facts, branch-stack-load authority, RV64 | Downstream non-goals; remain blocked. |

Exact blocker:

- A lower-level durable semantic instruction-result frame-slot
  write/materialization event source is missing.
- That source must prove `%t23 = bir.ne i32 %t22, 0`, value id `17`, type
  `i32`, was written/materialized into slot `#21`, object `#21`, offset `156`,
  size `4`, align `4`.
- It must not infer authority from final homes, storage plans, object/slot
  layout, value names, raw BIR adjacency, testcase shape, or `authority=none`
  moves.

Artifacts:

- `build/agent_state/479_step2_real_event_authority_contract/contract.md`

## Suggested Next

Execute Step 3 from `plan.md` as a routing/blocker packet:
`Route Real Event Authority Producer Blocker`.

Step 3 should record that no implementation is selected from current prepared
evidence and name the required lower-level producer surface for semantic
instruction-result frame-slot write/materialization events.

## Watchouts

- Do not implement a collector that turns `%t22 -> %t23` storage movement into
  semantic event authority.
- Do not add raw-shape, value-name, function-name, object-id, stack-offset, or
  testcase-specific inference.
- Keep interval/source-fact/branch-stack-load/RV64 consumers blocked until a
  real event-authority producer exists.

## Proof

Delegated proof:

```sh
git diff --check
```

Result: passed.
