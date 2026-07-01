Status: Active
Source Idea Path: ideas/open/500_semantic_global_static_gep_admission_producer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Classify Global/Static GEP Rows

# Current Packet

## Just Finished

Step 1 reproduced and classified the six global/static object GEP rows
separated by 499 Step 1. Durable evidence is recorded under
`build/agent_state/500_step1_global_static_gep_classification/`: four rows are
direct global/static object GEP candidates, one is a global pointer-field to
string-literal boundary, and one is a static-array/runtime-intrinsic boundary.

## Suggested Next

Execute Step 2 by defining the semantic global/static GEP admission contract
for the four direct candidate rows: `src/20000717-4.c`,
`src/20031214-1.c`, `src/20080424-1.c`, and `src/20120808-1.c`.

## Watchouts

- This is a BIR semantic producer packet, not RV64/MIR lowering.
- Do not infer global/static GEP authority from raw shape, names, labels, final
  homes, object files, relocations, lowered target behavior, or route-local
  slots.
- Keep `src/20051104-1.c` fail-closed or route it to string/global-pointer
  provenance first; direct global-object GEP authority does not prove the
  pointed string-literal object behind `s.name[s.len]`.
- Keep `src/ieee/copysign2.c` fail-closed or route it through runtime/string
  intrinsic consumer authority before admitting its static-array GEP use.
- Existing `bir::Global`, `LoadGlobalInst`/`StoreGlobalInst`, and lowerer
  `GlobalAddress` helpers are useful lower surfaces, but there is not yet a
  durable certificate tying global object, layout path, dynamic range, element
  byte range, and LIR producer coordinate together.

## Proof

Step 1 classification proof:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed > test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
