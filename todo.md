Status: Active
Source Idea Path: ideas/open/499_semantic_gep_local_memory_admission_producer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Classify GEP Local-Memory Admission Rows

# Current Packet

## Just Finished

Lifecycle closed completed idea 496 and activated new focused idea 499 for the
semantic `gep local-memory` admission producer selected by 496 Step 2.

## Suggested Next

Execute Step 1 by reproducing and classifying the 62-row semantic
`gep local-memory` family, including representatives `src/pr44468.c`,
`src/pr48571-1.c`, `src/pr65956.c`, `src/pr80421.c`, and `src/20000717-4.c`.

## Watchouts

- This is a BIR semantic producer packet, not RV64/MIR lowering.
- Do not infer GEP authority from raw shape, names, final homes, lowered target
  behavior, or route-local slots.
- Keep store local-memory, direct-call, scalar/local-memory, runtime,
  move-bundle, F128, stack-frame, and scalar-load work out of this packet.
- If a lower producer prerequisite is missing, route lifecycle to that
  prerequisite instead of implementing target inference.

## Proof

Lifecycle switch proof:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed > test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
