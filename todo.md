Status: Active
Source Idea Path: ideas/open/478_semantic_instruction_result_write_event_carrier.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Write Event Carrier Contract

# Current Packet

## Just Finished

Completed Step 2 contract for idea 478 by defining the authoritative semantic
instruction-result frame-slot write/materialization event carrier.

Accepted carrier contract:

| Fact group | Required for `available` carrier |
| --- | --- |
| Semantic result identity | Function, semantic producer block/instruction, result value id/name/type, opcode/kind, and operand identity for the instruction result, e.g. `%t23 = ne i32 %t22, 0`. |
| Event site | Explicit event block/instruction or phase/site naming where the write/materialization occurred. |
| Destination identity | Destination value id/name/type, frame slot id, stack object id, offset, size, and alignment. |
| Event source/result match | Event source/result must be the semantic result, not a different value or storage source. |
| Event authority | Explicit authority kind proving semantic instruction-result materialization/write into the destination frame slot. |
| Scope boundary | Carrier owns only the write/materialization event; path/no-clobber interval proof and downstream consumers are out of scope. |

Rejected / fail-closed cases:

| Shape | Disposition |
| --- | --- |
| Missing semantic result identity | `missing_semantic_result_identity`. |
| Missing event site or authority | `missing_event_authority`. |
| Final value home, storage plan, stack object, or frame-slot order only | Rejected as `missing_event_authority`; destination facts are not write events. |
| `%t22 -> %t23` `authority=none` storage move | `storage_only_move` / rejected; never semantic compare-result materialization. |
| Event source/result mismatch | `semantic_result_mismatch`. |
| Destination slot/object/offset/size/alignment mismatch | `destination_mismatch`. |
| Unknown carrier policy/unsupported event kind | `unsupported_event_authority`. |
| `%t22` select-result stack destination | Protected boundary; separate select-result/block-entry owner. |
| `%t1` / `%t7` pointer/provenance rows | Protected boundary; separate pointer/provenance owner. |
| `%t2` / `%t8` unsupported-terminator rows | Protected boundary; separate branch-site relationship owner. |
| Path/no-clobber interval, source-fact population, branch-stack-load authority, RV64 lowering | Downstream non-goals. |

Selected Step 3 packet:

- `Implement Or Route Write Event Carrier`.
- Add the smallest independent prepared write-event carrier/status surface and
  planner/checker for explicit event inputs.
- Positive coverage may use synthetic explicit semantic write-event inputs to
  prove the carrier can become `available`.
- Real `%t23` should remain unavailable with `missing_event_authority` unless
  a real producer supplies explicit authority.
- Fail-closed coverage must reject final-home/storage-only evidence,
  `%t22 -> %t23` `authority=none`, source/result mismatch, destination
  mismatch, protected boundaries, and raw-shape fixtures.
- Target files/tests if implemented:
  - `src/backend/prealloc/publication_plans.hpp`
  - `src/backend/prealloc/publication_plans.cpp`
  - `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
  - optional prepared printer files only if minimal status exposure is needed
  - `todo.md`
  - `test_after.log`
  - `build/agent_state/478_step3_semantic_write_event_carrier/**`

Artifacts:

- `build/agent_state/478_step2_semantic_write_event_carrier_contract/contract.md`

## Suggested Next

Execute Step 3 from `plan.md`: Implement Or Route Write Event Carrier. Add only
the independent event-carrier/status surface if current APIs can do so without
raw-shape inference or downstream consumption; otherwise record the exact API
blocker.

## Watchouts

- A status surface may prove explicit synthetic carriers, but it must not claim
  real `%t23` availability without explicit event authority.
- Final homes, storage plans, stack objects, and frame-slot order remain
  destination inputs only.
- `%t22 -> %t23` `authority=none` storage movement remains rejected.
- Do not implement path/no-clobber interval proof, source-fact population,
  branch-stack-load authority, or RV64 lowering in this idea.

## Proof

Delegated proof:

```sh
git diff --check
```

Result: passed.
