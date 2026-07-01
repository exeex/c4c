Status: Active
Source Idea Path: ideas/open/477_real_semantic_materialization_interval_fact_population.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Real Population Contract

# Current Packet

## Just Finished

Completed Step 2 contract for idea 477 by defining when real prepared evidence
may populate the idea 476 semantic materialization / interval status surface.

Accepted real population contract:

| Fact group | Required for real `available` |
| --- | --- |
| Semantic identity | Function, producer block/instruction, result value id/name/type, opcode or predicate, operand values/types, and producer kind for `%t23 = ne i32 %t22, 0`. Existing branch-condition identity may seed this input. |
| Destination identity | Destination value id/name/type, frame slot id `#21`, stack object `#21`, offset `156`, size `4`, and align `4`. Existing home/storage/object rows may seed this input. |
| Materialization/write event | A durable prepared event carrier saying the semantic result `%t23` was explicitly written/materialized into the destination frame slot/object, with event kind/site/authority and event source matching `%t23`. |
| Path coverage | Explicit dominance, path, or edge-scope proof from materialization/write event to the branch consumer at `logic.end.14`. |
| Same-slot exclusion | Slot `#21` writes in the event-to-consumer interval are classified and absent. |
| Effect safety | Calls/helpers/inline asm, publications, move bundles, and parallel copies in the interval are classified non-clobbering for slot/object `#21`. |

Rejected / fail-closed cases:

| Shape | Required status / disposition |
| --- | --- |
| Missing semantic identity | `missing_semantic_result_identity`. |
| Missing materialization/write event | `missing_materialization_event`. |
| `%t22 -> %t23` `authority=none` storage movement | Rejected as `materialization_value_mismatch` or equivalent storage-only non-event; never accepted as semantic materialization. |
| Event writes a different result/source | `materialization_value_mismatch`. |
| Event writes a different slot/object/offset/size/align | `materialization_destination_mismatch`. |
| Missing or non-covering path proof | `missing_path_validity` / `path_not_covering_consumer`. |
| Unknown or present same-slot writes | `same_slot_write_unknown` / `same_slot_write_found`. |
| Unknown or clobbering calls/helpers/inline asm | `call_or_helper_effect_unknown` / `call_or_helper_clobbers_slot`. |
| Unknown or clobbering publications/move bundles/parallel copies | Corresponding effect unknown/clobber status. |
| `%t22` select-result stack destination | Protected boundary; separate select-result/block-entry owner. |
| `%t1` / `%t7` pointer/provenance rows | Protected boundary; separate pointer/provenance owner. |
| `%t2` / `%t8` unsupported-terminator rows | Protected boundary; separate branch-site relationship owner. |
| Source-fact population, branch-stack-load authority, RV64 lowering | Downstream non-goals. |

Selected Step 3 packet:

- `Implement Or Route Real Semantic Fact Population`.
- First bounded packet: add or route a real prepared semantic
  materialization/write event carrier for scalar instruction-result frame-slot
  writes, then feed that explicit carrier into the existing idea 476
  `PreparedSemanticMaterializationInterval` status planner.
- Minimal target files/tests if implementation is justified:
  - `src/backend/prealloc/publication_plans.hpp`
  - `src/backend/prealloc/publication_plans.cpp`
  - `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
  - optional prepared printer files only if evidence exposure is required
  - `todo.md`
  - `test_after.log`
  - `build/agent_state/477_step3_real_semantic_fact_population/**`
- If current prepared producers cannot supply a durable semantic write-event
  carrier without raw-shape inference or storage-only move reuse, Step 3 must
  route/block and name the exact missing carrier.

Artifacts:

- `build/agent_state/477_step2_real_semantic_fact_population_contract/contract.md`

## Suggested Next

Execute Step 3 from `plan.md`: Implement Or Route Real Semantic Fact
Population. Attempt only the first real event-carrier packet, and stop with a
blocker if no durable semantic materialization/write event can be populated
without raw-shape inference.

## Watchouts

- Existing semantic identity and destination facts are necessary but not
  sufficient for availability.
- `%t22 -> %t23` `authority=none` storage movement remains rejected.
- Do not mark downstream `PreparedFrameSlotSourceFact`,
  `PreparedBranchStackLoadAuthority`, or RV64 consumers available.
- Preserve select-result stack-destination, pointer/provenance, and
  unsupported-terminator rows as separate owner families.

## Proof

Delegated proof:

```sh
git diff --check
```

Result: passed.
