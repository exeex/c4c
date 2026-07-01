Status: Active
Source Idea Path: ideas/open/496_semantic_lir_to_bir_admission_high_impact_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reclassify Semantic Admission After Scalar Local Loads

# Current Packet

## Just Finished

Lifecycle closed completed idea 483 and resumed idea 496. Idea 483 published
`local_array_scalar_local_loads`; the active cleanup should now reclassify the
semantic admission bucket using that fact surface.

## Suggested Next

Execute Step 1 by refreshing semantic `lir_to_bir` admission classification
after scalar local-load facts, separating fact-consuming downstream work from
remaining producer gaps.

## Watchouts

- Consumers should use `local_array_scalar_local_loads`; do not re-derive local
  object, element offset, layout, range, provenance, exact-address checks, or
  scalar load identity from lower surfaces, final homes, raw testcase shape,
  names, or target behavior.
- If a remaining failure lacks producer authority, route to a producer-owned
  idea before RV64/MIR lowering.
- Do not fold move-bundle materialization, F128, runtime mismatch, or global
  stack-frame work into this packet unless refreshed bucket evidence selects
  that owner.

## Proof

Lifecycle switch proof:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed > test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
