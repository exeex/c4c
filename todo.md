Status: Active
Source Idea Path: ideas/open/474_prepared_frame_slot_materialization_no_clobber_facts.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Prepared Source-Fact Contract

# Current Packet

## Just Finished

Completed Step 2 contract for idea 474. The prepared source-fact carrier must
publish independent frame-slot materialization/write and no-clobber facts; it
must not mark downstream branch-stack-load authority available.

Record key and payload contract:

| Fact group | Required fields |
| --- | --- | --- |
| Record identity | Function id, target/consumer block label, optional instruction index, target value id/name, target type, frame slot id, stack object id, stack offset, size, and alignment. |
| Source value identity | Source value kind, source value id/name when named, source type/width, producer kind, producer block/instruction or edge site, and source home if relevant. |
| Materialization/write event | Event kind, event site, source value, destination frame slot/object, destination offset/size/align, and value/type/width match status. |
| Path validity | Source event reaches the consumer site for the declared scope, or records explicit path/edge scope; unknown paths are unavailable. |
| Same-slot exclusion | Target slot/object interval contains no same-slot write, or records the first same-slot write/unknown status. |
| Effect safety | Calls/helpers/inline asm between source event and consumer site are classified safe, clobbering, or unknown for the slot/object. |
| Prepared event effects | Publications, move bundles, and parallel copies are classified non-clobbering, clobbering, or unknown for the target slot/object. |
| Consumer boundary | The record is a source fact only. Later certificate/authority packets may consume it; this plan does not set branch-stack-load availability or RV64 behavior. |

Status vocabulary:

| Status | Meaning |
| --- | --- |
| `available` | All required identity, materialization/write, path, same-slot, effect, and prepared-event non-clobber facts are explicit. |
| `missing_source_value` | No source value/source producer is available. |
| `missing_materialization_event` | No concrete event writes/materializes the source into the target frame slot/object. |
| `materialization_slot_mismatch` | Event exists but destination slot/object/offset/size/align does not match the record target. |
| `materialization_value_mismatch` | Event source value/type/width does not match the target current value requirement. |
| `missing_path_validity` | No dominance/path/edge-scope proof links the event to the consumer site. |
| `path_not_covering_consumer` | Path proof exists but does not cover the required consumer site/path. |
| `same_slot_write_found` | A write to the same slot/object occurs in the interval. |
| `same_slot_write_unknown` | Same-slot writes are not analyzed for the interval. |
| `call_or_helper_effect_unknown` | A call/helper/inline-asm effect is present or possible but not classified safe. |
| `call_or_helper_clobbers_slot` | A call/helper/inline-asm effect may clobber the target slot/object. |
| `publication_effect_unknown` / `publication_clobbers_slot` | Prepared publication effects are unknown or clobbering for the target. |
| `move_bundle_effect_unknown` / `move_bundle_clobbers_slot` | Move-bundle effects are unknown or clobbering for the target. |
| `parallel_copy_effect_unknown` / `parallel_copy_clobbers_slot` | Parallel-copy effects are unknown or clobbering for the target. |
| `unsupported_boundary` | Row belongs to select-result stack destination, pointer/provenance, unsupported terminator, or another non-474 owner. |

Positive and negative contract:

| Case | Required outcome |
| --- | --- |
| Explicit scalar event writes/materializes a named value into the matching frame slot, path covers consumer, and all interval effects are proven non-clobbering. | Publish `available` source fact. |
| `%t23` representative with only home/storage/object identity and no write event. | Publish missing/unavailable status; do not infer availability. |
| Any mismatch in source value, slot/object, offset, size, alignment, type, or path. | Fail closed with the corresponding status. |
| Any unclassified same-slot write, call/helper/inline-asm, publication, move-bundle, or parallel-copy effect. | Fail closed as unknown/clobbering. |
| `%t22` select-result stack destination, `%t1`/`%t7` pointer/provenance, `%t2`/`%t8` unsupported terminator rows. | Preserve as unsupported/separate boundaries. |

Artifacts:

- `build/agent_state/474_step2_source_fact_contract/contract.md`

## Suggested Next

Execute Step 3 from `plan.md`: Implement Or Route Prepared Source-Fact
Producer.

Selected first packet: add the smallest prepared carrier/status surface for
frame-slot materialization/no-clobber source facts.

Suggested owned files:

- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
- optional prepared printer files only if new record/status exposure is needed
- `todo.md`
- `test_after.log`
- `build/agent_state/474_step3_source_fact_carrier/**`

Step 3 should first prove status/carrier behavior with focused inputs. It may
publish unavailable records for representative missing facts. It should publish
`available` only in a fixture where explicit materialization/write, path
validity, same-slot exclusion, effect safety, and prepared-event non-clobber
inputs are all present. If those inputs cannot be supplied without raw-shape
inference, route/block instead.

Proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Step 3 must not mark `PreparedBranchStackLoadAuthority` available.
- Do not implement RV64 branch-load emission in this producer plan.
- Do not infer source facts from stack homes/storage, offsets, object ids, raw
  BIR, value names, block labels, function names, testcase names, or dump
  order.
- Keep pointer-value/provenance, select-result stack-destination, and
  unsupported-terminator boundaries separate.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Classification proof:

```sh
git diff --check
```

Result: passed.
