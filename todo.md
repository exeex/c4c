Status: Active
Source Idea Path: ideas/open/499_semantic_gep_local_memory_admission_producer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Classify GEP Local-Memory Admission Rows

# Current Packet

## Just Finished

Step 1 reproduced and classified the 62-row semantic `gep local-memory`
admission family, then repaired the classification so direct available
candidates are local-memory only. Evidence in
`build/agent_state/499_step1_gep_local_memory_classification/` records the row
list, classification counts, representative source shapes, available candidate
shapes, fail-closed boundaries, and first owner.

## Suggested Next

Execute Step 2 by defining the BIR semantic GEP local-memory admission contract
for direct local-object candidates before implementation. The first contract
target should cover local-memory shapes represented by `src/pr80421.c`,
`src/930614-2.c`, and `src/pr24851.c`, while preserving fail-closed statuses
for global/static object GEPs, pointer/formal provenance, runtime/string
intrinsic, aggregate/member, flexible-layout, alias, and variadic boundaries.

## Watchouts

- This is a BIR semantic producer packet, not RV64/MIR lowering.
- Do not infer GEP authority from raw shape, names, final homes, lowered target
  behavior, or route-local slots.
- Step 1 found only 3 direct local-object candidate rows. The 6 global/static
  object GEP rows are separated as later-owner boundaries, and the other 53 rows
  remain fail-closed or prerequisite-owned until explicit producer authority
  exists.
- Keep store local-memory, direct-call, scalar/local-memory, runtime,
  move-bundle, F128, stack-frame, and scalar-load work out of this packet.
- If a lower producer prerequisite is missing, route lifecycle to that
  prerequisite instead of implementing target inference.

## Proof

Step 1 classification proof:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed > test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
