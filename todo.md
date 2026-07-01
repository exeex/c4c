Status: Active
Source Idea Path: ideas/open/476_semantic_instruction_result_frame_slot_materialization_facts.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Semantic Materialization Interval Contract

# Current Packet

## Just Finished

Completed Step 2 contract for idea 476. A bounded Step 3 producer packet is
justified: add the smallest independent prepared semantic materialization /
interval record-status surface. The packet must not populate source-fact rows,
branch-stack-load authority, or RV64 lowering.

Positive contract:

| Fact group | Required for `available` |
| --- | --- |
| Semantic instruction-result identity | Function, producer block/instruction, result value id/name/type, opcode or predicate, operand values/types, and semantic producer kind. |
| Destination frame-slot identity | Destination value id/name/type, frame slot id, stack object id, offset, size, and alignment. |
| Materialization/write event | Explicit event kind/site and authority proving the semantic result was written/materialized into the destination frame slot/object. |
| Event/result match | Event source/result must match the semantic result, and event destination must match the target frame slot/object/offset/size/alignment. |
| Path validity | Dominance, path, or edge-scope proof must cover the consumer. |
| Same-slot exclusion | Same-slot writes in the interval must be classified and absent. |
| Effect safety | Calls/helpers/inline asm, publications, move bundles, and parallel copies must be classified non-clobbering for the slot/object. |

Negative / fail-closed cases:

| Shape | Disposition |
| --- | --- |
| Missing semantic result identity | `missing_semantic_result_identity`. |
| Missing materialization/write event | `missing_materialization_event`. |
| Event source/result mismatch | `materialization_value_mismatch`. |
| Event destination mismatch | `materialization_destination_mismatch`. |
| Missing or invalid path/edge proof | `missing_path_validity` or `path_not_covering_consumer`. |
| Unknown/found same-slot write | `same_slot_write_unknown` / `same_slot_write_found`. |
| Unknown/clobbering calls/helpers/inline asm | `call_or_helper_effect_unknown` / `call_or_helper_clobbers_slot`. |
| Unknown/clobbering publications, move bundles, parallel copies | Corresponding effect unknown/clobber status. |
| `%t22 -> %t23` storage-only move with `authority=none` | Rejected; it is not semantic materialization of `%t23 = ne i32 %t22, 0`. |
| Protected `%t22`, `%t1/%t7`, `%t2/%t8` boundary rows | `unsupported_boundary` or skipped with explicit rationale; do not fold into idea 476. |

Selected Step 3 packet:

- Implement or route the independent semantic materialization/interval
  record-status surface.
- Target files/tests if implemented:
  - `src/backend/prealloc/publication_plans.hpp`
  - `src/backend/prealloc/publication_plans.cpp`
  - `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
  - optional prepared printer files only if minimal status exposure is needed
  - `todo.md`
  - `test_after.log`
  - `build/agent_state/476_step3_semantic_materialization_interval_surface/**`
- Positive coverage may use explicit synthetic inputs. Real `%t23` should
  remain fail-closed until producer facts exist.
- Negative coverage must reject storage-only moves, destination/source
  mismatch, missing path, same-slot writes, unknown/clobbering effects, and
  protected boundaries.

Artifacts:

- `build/agent_state/476_step2_semantic_materialization_interval_contract/contract.md`

## Suggested Next

Execute Step 3 from `plan.md`: Implement Or Route Semantic Materialization
Producer. Add the smallest independent prepared record/status surface if the
current APIs can do so without downstream source-fact population or raw-shape
inference; otherwise record the exact API blocker.

## Watchouts

- The Step 3 surface may prove synthetic explicit records, but must not claim
  real `%t23` availability without a real materialization/write event and
  interval proof.
- Do not reuse `%t22 -> %t23` storage movement as semantic materialization.
- Do not populate `PreparedFrameSlotSourceFact`, branch-stack-load authority,
  or RV64 lowering in idea 476.
- Keep select-result, pointer/provenance, and unsupported-terminator rows in
  separate owner families.

## Proof

Delegated proof:

```sh
git diff --check
```

Result: passed.
