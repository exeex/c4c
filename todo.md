Status: Active
Source Idea Path: ideas/open/481_semantic_result_frame_slot_materialization_point_producer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Focused Materialization Point Inputs

# Current Packet

## Just Finished

Completed Step 1 for resumed idea 481 by auditing focused
materialization-point inputs from the scalar compare frame-slot destination
probe.

Focused explicit facts:

| Fact | Evidence | Classification |
| --- | --- | --- |
| Semantic instruction-result identity | `%t2 = bir.ne i32 %t0, %t1` | Explicit. |
| Semantic producer class | `store_source ... source=%t2 ... source_producer=binary source_producer_block=entry source_producer_inst=4 source_binary=yes` | Explicit Step 2 contract input. |
| Store site | `bir.store_local %lv.comparison, i32 %t2`; store-source row `block=entry inst=5` | Explicit. |
| Destination object | `object #2 func=main name=%lv.comparison source_kind=local_slot type=i32 size=4 align=4 address_exposed=yes requires_home_slot=yes permanent_home_slot=yes` | Explicit. |
| Destination frame slot | `slot #0 object_id=2 func=main offset=0 size=4 align=4 fixed_location=yes` | Explicit slot/object/layout match. |
| Store-site frame-slot access | `access block=entry inst_index=5 base=frame_slot stored=%t2 frame_slot=#0 offset=0 size=4 align=4` | Explicit. |
| Result home/storage | `home %t2 value_id=2 kind=stack_slot slot_id=5 offset=24`; `storage %t2 ... spill_slot=slot#5+stack24` | Explicit placement, but not materialization authority by itself. |
| Downstream authority rows | No available semantic write-event, semantic interval, frame-slot source fact, or branch-stack-load authority asserted by the focused probe. | Correct fail-closed boundary. |

Materialization-point authority still missing:

- There is no durable prepared record that ties semantic producer `%t2`, store
  site `entry` / instruction `5`, destination object `%lv.comparison` / object
  `#2`, destination frame slot `#0`, store-source `source_producer=binary`, and
  store-site access `stored=%t2 frame_slot=#0` into one semantic result
  frame-slot materialization point.
- Final homes, spill storage, stack object layout, value names, testcase names,
  and dump order remain insufficient as standalone authority.

Step 2 contract readiness:

- Step 2 can define a bounded producer contract from the focused inputs.
- The contract should accept only a local store of a semantic scalar
  instruction result with available prepared `store_source`, matching
  store-site frame-slot access, and matching destination object/slot/layout.
- The contract should reject missing/mismatched store-source rows, missing or
  mismatched store-site access, destination mismatch, raw-shape inference,
  final-home-only evidence, storage-only `authority=none` movement, select
  result boundaries, pointer/provenance rows, unsupported terminator rows, and
  downstream consumer claims.

Why this is distinct from the monolithic `930930-1` route:

- The focused probe isolates one scalar binary result and one local frame-slot
  destination.
- It does not require `%t22` select-result stack-destination ownership,
  pointer/provenance repair, unsupported-terminator relationship repair,
  `%t22 -> %t23 authority=none` storage movement, downstream interval/source
  facts, branch-stack-load authority, or RV64 consumption.

Artifact:

- `build/agent_state/481_step1_focused_materialization_point_inputs/audit.md`

## Suggested Next

Execute Step 2 by defining the focused materialization-point contract. The
contract should bind semantic producer identity, available binary
`store_source`, store site, destination object/slot/layout, and store-site
frame-slot access into a single explicit materialization-point authority
record, while keeping downstream consumers out of scope.

## Watchouts

- Do not treat `%t2` final stack home or spill slot as destination
  materialization authority.
- Do not infer the materialization point from raw BIR adjacency, value names,
  object ids alone, testcase shape, or dump order.
- Do not resume the old `%t23`/`930930-1` route in Step 2.
- Keep semantic interval, frame-slot source facts, branch-stack-load authority,
  and RV64 consumption as downstream non-goals.

## Proof

Delegated proof:

```sh
git diff --check
```

Result: passed.
