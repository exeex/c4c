Status: Active
Source Idea Path: ideas/open/478_semantic_instruction_result_write_event_carrier.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Semantic Write Event Carrier Inputs

# Current Packet

## Just Finished

Completed Step 1 audit for idea 478 by classifying prepared event, move,
publication, instruction-result, value-home, and frame-slot inputs for an
authoritative semantic write/materialization event carrier for `930930-1`
`%t23` slot `#21`.

Carrier input audit:

| Input surface | Representative evidence | Carrier verdict |
| --- | --- | --- |
| Semantic instruction result | `logic.end.14` has `%t23 = bir.ne i32 %t22, 0`; prepared branch condition records `condition=%t23 compare=ne i32 %t22, 0`. | Usable semantic identity input. |
| Consumer branch | `bir.cond_br i32 %t23, block_5, block_6`; branch condition has true/false labels. | Useful context, not write authority. |
| Value home | `home %t23 value_id=17 kind=stack_slot slot_id=21 offset=156`. | Usable destination value/slot input, not event authority. |
| Storage plan | `storage %t23 value_id=17 encoding=frame_slot ... slot#21+stack156`. | Final placement only; insufficient as write event. |
| Stack object / frame slot layout | `object #21 func=f name=%t23 type=i32 size=4 align=4`; frame slot order shows slot `#21 offset=156 size=4 align=4`. | Usable destination layout input, not event authority. |
| Move bundle `%t22 -> %t23` | `move_bundle phase=before_instruction authority=none block_index=11 instruction_index=3`; `move from_value_id=16 to_value_id=17 destination_storage=stack_slot`. | Rejected storage-only movement; not semantic compare-result materialization. |
| Block-entry publications into `%t22` | `status=unsupported_destination_storage` for `%t22` stack destination. | Protected select-result boundary; not `%t23` write authority. |
| Parallel copies into `%t22` | Authoritative select-materialization copies target `%t22`, not `%t23`. | Out of scope for `%t23` event carrier. |
| Prepared branch-stack-load authority | `%t23` condition row remains `status=missing_policy`. | Downstream signal only; not event authority. |
| Existing semantic interval planner | Can accept explicit synthetic facts, but has no real event collector/caller. | Requires this idea's carrier before real population can proceed. |

Protected boundaries:

| Boundary row | Evidence | Disposition |
| --- | --- | --- |
| `%t22` select-result stack destination | `select_chain root_is_select=yes`; block-entry publications to `%t22` are `unsupported_destination_storage`. | Separate select-result/block-entry owner. |
| `%t1` / `%t7` pointer/provenance rows | Prior families remain pointer/provenance gaps. | Out of scope. |
| `%t2` / `%t8` unsupported-terminator rows | Branch-stack-load records remain `unsupported_terminator`. | Out of scope. |
| Interval/source-fact/branch-authority/RV64 consumers | Need event carriers and later interval/source facts first. | Downstream non-goals. |

First Step 2 contract target:

- Define an authoritative semantic instruction-result frame-slot
  write/materialization event carrier.
- Required carrier fields should include function, event block/instruction,
  semantic producer identity, result value id/name/type, opcode/kind,
  destination frame slot/object/offset/size/alignment, and event authority.
- Negative cases must reject missing event authority, storage-only moves,
  result mismatch, destination mismatch, select-result stack destinations,
  pointer/provenance gaps, unsupported terminators, and raw-shape/final-home
  inference.

Artifacts:

- `build/agent_state/478_step1_semantic_write_event_carrier_audit/audit.md`
- `build/agent_state/478_step1_semantic_write_event_carrier_audit/evidence_snippets.txt`

## Suggested Next

Execute Step 2 from `plan.md`: Define Write Event Carrier Contract. The
contract should separate usable identity/destination inputs from the
authoritative event carrier and decide whether a bounded Step 3 producer packet
is justified.

## Watchouts

- Do not accept final homes, storage plans, stack objects, or frame-slot order
  as semantic write authority.
- Do not reuse `%t22 -> %t23` `authority=none` storage movement as semantic
  materialization.
- Do not fold path/no-clobber interval proof, source-fact population,
  branch-stack-load authority, or RV64 lowering into this event-carrier idea.
- Keep select-result stack-destination, pointer/provenance, and
  unsupported-terminator boundaries separate.

## Proof

Delegated proof:

```sh
git diff --check
```

Result: passed.
