Status: Active
Source Idea Path: ideas/open/500_semantic_global_static_gep_admission_producer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Classify Global/Static GEP Rows

# Current Packet

## Just Finished

Lifecycle closed completed idea 499 and activated new focused idea 500 for the
global/static GEP producer boundary separated by 499 Step 1 and left as
residual by 499 Step 3.

## Suggested Next

Execute Step 1 by reproducing and classifying the six global/static object GEP
rows, including `src/20000717-4.c`, `src/20031214-1.c`,
`src/20051104-1.c`, `src/20080424-1.c`, `src/20120808-1.c`, and
`src/ieee/copysign2.c`.

## Watchouts

- This is a BIR semantic producer packet, not RV64/MIR lowering.
- Do not infer global/static GEP authority from raw shape, names, labels, final
  homes, object files, relocations, lowered target behavior, or route-local
  slots.
- Keep local-object GEPs, pointer/formal provenance, runtime/string,
  aggregate/member, variadic, store, call, F128, move-bundle, stack-frame, and
  scalar-load work out of this packet.
- If a lower producer prerequisite is missing, route lifecycle to that
  prerequisite instead of implementing target inference.

## Proof

Lifecycle switch proof:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed > test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
