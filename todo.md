Status: Active
Source Idea Path: ideas/open/475_prepared_frame_slot_source_fact_population.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Real Source-Fact Population Inputs

# Current Packet

## Just Finished

Closed idea 474 as complete for the independent prepared frame-slot
source-fact carrier/status surface. The carrier, planner, availability
predicate, and collector exist; protected boundary rows remain
`unsupported_boundary`.

The new active idea 475 owns real source-fact population for materialization/
write events, path/dominance validity, same-slot write exclusion,
call/helper/inline-asm effect safety, and publication/move/parallel-copy
non-clobber facts. Downstream branch-stack-load authority and RV64 branch-load
consumption remain blocked.

## Suggested Next

Execute Step 1 from `plan.md`: audit real source-fact population inputs for
`%t23` slot `#21` and adjacent protected boundary rows.

Suggested artifact directory:
`build/agent_state/475_step1_source_fact_population_audit/`.

## Watchouts

- Do not edit implementation files during Step 1.
- Do not implement RV64 branch-load emission in this producer plan.
- Do not directly populate available branch-stack-load authority records.
- Do not infer source facts from stack homes/storage, offsets, object ids, raw
  BIR, value names, block labels, function names, testcase names, or dump
  order.
- Keep pointer-value/provenance, select-result stack-destination, and
  unsupported-terminator boundaries separate.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Lifecycle transition proof:

```sh
git diff --check
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed
python3 scripts/plan_review_state.py show
```

Result: passed.
