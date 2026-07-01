Status: Active
Source Idea Path: ideas/open/477_real_semantic_materialization_interval_fact_population.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Real Semantic Fact Population Inputs

# Current Packet

## Just Finished

Closed idea 476 as complete for the independent semantic materialization /
interval status surface. Real `%t23 = ne i32 %t22, 0` into slot `#21` still
lacks producer-populated materialization/write and path/no-clobber interval
facts.

The new active idea 477 owns real semantic materialization/interval fact
population. Source-fact population, branch-stack-load authority, and RV64
branch-load consumption remain blocked.

## Suggested Next

Execute Step 1 from `plan.md`: audit real semantic fact population inputs for
`%t23` slot `#21` and adjacent protected boundary rows.

Suggested artifact directory:
`build/agent_state/477_step1_real_semantic_fact_population_audit/`.

## Watchouts

- Do not edit implementation files during Step 1.
- Do not implement RV64 branch-load emission in this producer plan.
- Do not directly populate source-fact or branch-stack-load authority records.
- Do not reuse `%t22 -> %t23` stack moves as semantic materialization evidence
  when `authority=none`.
- Do not infer materialization/no-clobber facts from raw BIR, stack homes,
  storage, offsets, object ids, value names, function names, testcase names, or
  dump order.
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
