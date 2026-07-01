Status: Active
Source Idea Path: ideas/open/476_semantic_instruction_result_frame_slot_materialization_facts.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Semantic Materialization Interval Inputs

# Current Packet

## Just Finished

Completed Step 1 audit for idea 476. The prepared evidence has semantic
instruction-result identity and target frame-slot identity for `%t23`, but it
does not yet have materialization/write or path/no-clobber interval inputs.

Audit table:

| Fact group | Existing evidence for `%t23` slot `#21` | Classification |
| --- | --- | --- |
| Semantic result identity | BIR/prepared dump shows `%t23 = bir.ne i32 %t22, 0`; prepared branch condition records `condition=%t23 compare=ne i32 %t22, 0` at `logic.end.14`. | Accepted identity input; not itself a materialization event. |
| Target frame-slot identity | `%t23` has `value_id=17`, stack home slot `#21`, stack offset `156`, object `#21`, type `i32`, size/alignment `4`. | Accepted destination identity input. |
| Materialization/write event | No prepared record says the `%t23` compare result is written or materialized into slot `#21`. | Missing first producer fact. |
| `%t22 -> %t23` storage move | `move_bundle phase=before_instruction authority=none block_index=11 instruction_index=3` moves `from_value_id=16` to `to_value_id=17` with stack-slot destination. | Rejected. It is storage movement from `%t22`, not semantic compare-result materialization for `%t23`. |
| Path/dominance or edge-scope validity | CFG and branch site are known, but no record ties a materialization/write event to reach `logic.end.14`. | Missing interval fact. |
| Same-slot write exclusion | No classifier proves slot `#21` is not overwritten between materialization and consumer. | Missing interval fact. |
| Call/helper/inline-asm safety | Callsite rows exist, but no slot `#21` effect-safety record exists. | Missing interval fact. |
| Publication/move/parallel-copy non-clobber | Parallel copies, block-entry publications, and move bundles are visible, but no slot `#21` clobber classification exists. | Missing interval fact. |

Protected boundary rows:

| Boundary row | Classification |
| --- | --- |
| `%t22` select-result stack destination | Separate select-result/block-entry stack-destination owner. |
| `%t1` / `%t7` pointer/provenance rows | Separate pointer-value/provenance owner. |
| `%t2` / `%t8` unsupported-terminator rows | Separate branch-site relationship / terminator owner. |
| Source-fact population / branch-stack-load authority / RV64 consumption | Remain blocked until lower-level semantic materialization and interval facts exist. |

First exact Step 2 target: define a semantic materialization/interval contract.
The likely blocker is missing producer surfaces for semantic instruction-result
frame-slot materialization/write records and path/no-clobber interval records;
`%t23` identity and destination slot identity are available, but insufficient.

Artifacts:

- `build/agent_state/476_step1_semantic_materialization_interval_audit/audit.md`

## Suggested Next

Execute Step 2 from `plan.md`: Define Semantic Materialization Interval
Contract. The contract should decide whether a bounded producer packet can add
records/statuses for semantic instruction-result materialization/write events
and path/no-clobber intervals, or whether another lower-level owner is still
missing.

## Watchouts

- Do not reuse `%t22 -> %t23` storage movement as semantic materialization.
- Do not infer materialization or no-clobber facts from raw BIR shape, final
  stack homes/storage, offsets, object ids, value names, function names,
  testcase shape, or dump order.
- Do not populate source-fact records, branch-stack-load authority, or RV64
  lowering in idea 476.
- Keep select-result, pointer/provenance, and unsupported-terminator rows in
  their own owner families.

## Proof

Delegated proof:

```sh
git diff --check
```

Result: passed.
