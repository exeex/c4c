Status: Active
Source Idea Path: ideas/open/481_semantic_result_frame_slot_materialization_point_producer.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Focused Materialization Point Contract

# Current Packet

## Just Finished

Completed Step 2 for idea 481 by defining the focused materialization-point
contract from the scalar compare destination probe.

Contract decision:

- A bounded producer packet exists.
- `store_source status=available source_producer=binary` plus matching
  store-site frame-slot access and destination object/slot/layout is enough to
  publish a focused materialization-point authority record.
- This authority must be produced from prepared rows, not raw BIR shape, final
  homes, storage, object names, testcase shape, or dump order.

Accepted focused evidence:

| Field | Required evidence |
| --- | --- |
| Function | Same prepared function for all participating rows. |
| Semantic producer | Binary semantic instruction result, e.g. `%t2 = bir.ne i32 %t0, %t1`. |
| Producer authority | `store_source status=available source=%t2 source_producer=binary source_producer_block=entry source_producer_inst=4 source_binary=yes`. |
| Store site | Store-source row identifies the local store site, e.g. `block=entry inst=5`. |
| Store-site access | Prepared addressing row at the same store site with `base=frame_slot stored=%t2`. |
| Destination object | Prepared stack object for the stored local destination, e.g. `%lv.comparison` object `#2`, type `i32`, size `4`, align `4`. |
| Destination slot/layout | Prepared frame slot matching the access row and object, e.g. slot `#0`, object `#2`, offset `0`, size `4`, align `4`. |
| Result identity | Result value id/name/type for the semantic source, e.g. `%t2` / value id `2` / `i32`. |

Required materialization-point record:

- function name;
- semantic source value id/name/type;
- semantic producer kind and producer block/instruction;
- store block/instruction;
- destination object id/name/type;
- destination frame slot id;
- destination offset/size/alignment;
- authority/status `available`;
- authority source `store_source_binary_frame_slot_store` or equivalent stable
  spelling.

Fail-closed statuses:

| Status | Trigger |
| --- | --- |
| `missing_store_source_authority` | No prepared store-source row exists for the store site. |
| `unsupported_source_producer` | Store source is unavailable, unknown, immediate-only, select-result, load-only, cast-only, or any non-binary producer outside this focused packet. |
| `source_result_mismatch` | Store-source value does not match the semantic producer result or the frame-slot access stored value. |
| `missing_frame_slot_access` | No prepared addressing/access row exists at the store site. |
| `destination_mismatch` | Access row, stack object, frame slot, offset, size, or alignment disagree. |
| `storage_only_move` | Evidence is only a value/storage movement such as `authority=none`, without store-source plus access authority. |
| `protected_select_result_boundary` | Candidate source or destination belongs to select-result stack-destination publication rather than this scalar binary store contract. |
| `protected_pointer_or_terminator_boundary` | Candidate row belongs to pointer/provenance or unsupported-terminator families. |
| `raw_shape_inference` | Candidate would be accepted only from raw BIR adjacency, final homes, spill slots, object ids, names, testcase shape, or dump order. |

Artifact:

- `build/agent_state/481_step2_focused_materialization_point_contract/contract.md`

## Suggested Next

Execute Step 3 by implementing the focused producer surface for
materialization-point authority records. Target files should be limited to
`src/backend/prealloc/publication_plans.hpp`,
`src/backend/prealloc/publication_plans.cpp`, prepared printer files only if
durable evidence exposure is needed, focused backend BIR/printer tests,
`todo.md`, `test_after.log`, and
`build/agent_state/481_step3_focused_materialization_point_producer/**`.

Step 3 proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Do not publish semantic interval, `PreparedFrameSlotSourceFact`,
  `PreparedBranchStackLoadAuthority`, or RV64 lowering authority from the
  materialization-point record.
- Do not accept final home, spill storage, object layout, names, testcase
  shape, or raw BIR adjacency as standalone materialization authority.
- Keep storage-only `authority=none`, select-result stack-destination,
  pointer/provenance, unsupported-terminator, and downstream consumer rows
  fail-closed.
- Do not return to the monolithic `930930-1` route for Step 3 coverage.

## Proof

Delegated proof:

```sh
git diff --check
```

Result: passed.
