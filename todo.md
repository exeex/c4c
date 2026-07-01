Status: Active
Source Idea Path: ideas/open/496_semantic_lir_to_bir_admission_high_impact_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reclassify Semantic Admission After Scalar Local Loads

# Current Packet

## Just Finished

Step 1 reclassified the semantic `lir_to_bir` admission bucket after idea 483
published `local_array_scalar_local_loads`. The refreshed evidence records a
`373`-row semantic bucket from the fresh full scan, disjoint semantic-family
counts, representative rows, and the local-load disposition in
`build/agent_state/496_step1_semantic_admission_after_scalar_local_loads/`.

## Suggested Next

Execute Step 2 by selecting the next producer or consumer packet. The next
packet should either pick a producer-owned semantic family such as `gep
local-memory` (`62`), `store local-memory` (`58`), direct-call (`52`), or
`scalar/local-memory` (`49`), or explicitly hand off a downstream consumer that
uses `local_array_scalar_local_loads`.

## Watchouts

- Consumers should use `local_array_scalar_local_loads`; do not re-derive local
  object, element offset, layout, range, provenance, exact-address checks, or
  scalar load identity from lower surfaces, final homes, raw testcase shape,
  names, or target behavior.
- The full `79`-row load local-memory family is not solved as a group:
  representative local-array element-load rows can now consume scalar-load facts,
  while aggregate/member, variadic, global, pointer-derived, and integer-pointer
  round-trip rows remain producer-owned boundaries.
- If a remaining failure lacks producer authority, route to a producer-owned idea
  before RV64/MIR lowering.
- Do not fold move-bundle materialization, F128, runtime mismatch, or global
  stack-frame work into this packet unless refreshed bucket evidence selects
  that owner.

## Proof

Step 1 classification proof:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed > test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
