Status: Active
Source Idea Path: ideas/open/488_bir_dynamic_local_array_consumer_coordinate_prepared_exposure.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Consumer Coordinate Exposure Inputs

# Current Packet

## Just Finished

Completed Step 1 audit for idea 488.

Audit result:

| Surface | Available fields | Missing coordinate/exposure | Classification |
| --- | --- | --- | --- |
| BIR local-array path records | Path result, source object, derivation result, dynamic index, element type/size/count, byte offset, and status. | Consumer block label, consumer instruction index, operation role, and stable prepared lookup key. | Good payload, missing coordinate. |
| Local GEP publisher | Recognizes supported dynamic local-array pointer/aggregate paths and publishes `missing_index_range_proof` records. | Caller does not pass block/instruction coordinate into `publish_local_array_path_record`. | First producer seam for coordinate capture. |
| BIR lowered instruction stream | Blocks and instructions have labels/positions after lowering. | No back-reference from function-level `local_array_element_paths` to the instruction site. | Possible unique-match exposure only if fail-closed. |
| Prepared object traversal | `PreparedObjectTraversalEvent` exposes block index, instruction index, prepared block, BIR block, and instruction pointer. | No local-array path classification or lookup attached to traversal events. | Good prepared exposure surface after BIR coordinate exists. |
| Prepared control-flow lookup | Prepared block/branch facts share function and block identity. | No local-array consumer lookup table keyed to prepared control-flow coordinates. | Later lookup consumer. |

Conclusion: a bounded exposure contract exists, but it must begin with durable
consumer-coordinate capture/exposure. The clean first implementation shape is
to thread block label and instruction index into local-memory/GEP lowering and
publish coordinate-bearing local-array path records. A post-lowering prepared
collector could be acceptable only if it matches path result values to exact BIR
instruction events uniquely and fails closed for missing, duplicate, or
mismatched candidates.

Supporting artifact:

- `build/agent_state/488_step1_consumer_coordinate_exposure_inputs/audit.md`

## Suggested Next

Execute Step 2: define the consumer-coordinate/prepared-exposure contract,
including required fields, supported operation roles, lookup keys, and
fail-closed cases for missing/duplicate/mismatched coordinate matches.

## Watchouts

- Do not populate proof-source, path/dominance, or no-clobber facts in this
  coordinate exposure packet.
- Do not infer coordinates from dump order, testcase names, value names, branch
  proximity, loop shape, final homes, or RV64 target behavior.
- Do not mark dynamic range proofs available, change idea 486 checker
  vocabulary, package idea 484 records, consume scalar local loads, or change
  RV64/MIR lowering.
- Do not touch `review/`, canonical logs, baseline files, implementation files,
  or tests until an executor receives a bounded packet.
- Prefer explicit coordinate capture at the local-array path producer over
  post-hoc matching; if post-hoc matching is selected, require unique exact
  instruction/result evidence and fail closed otherwise.
- Do not treat prepared traversal coordinates alone as proof that a traversal
  event is the local-array consumer; the path record must be explicitly linked.

## Proof

Step 1 validation:

```sh
git diff --check
```

Result: passed.
