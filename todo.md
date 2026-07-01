Status: Active
Source Idea Path: ideas/open/493_dynamic_local_array_selected_proof_edge_path_record_collector_population.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Collector Inputs And Matching Keys

# Current Packet

## Just Finished

Lifecycle split closed idea 492 as complete for the selected proof-edge path
record/status API and activated idea 493 for the next first owner: real
collector/population of `local_array_selected_proof_edge_paths` records.

## Suggested Next

Execute Step 1: audit prepared branch/compare facts, successor labels,
reachability/dominance helpers, local-array path records, and exact
`lir_producer_lookup_key` matching. Record whether collector population is
bounded or identify the exact lower blocker.

## Watchouts

- Keep dynamic-index interval effect/no-clobber classification out of this
  runbook; it remains a later owner.
- Do not populate idea 489 proof facts or idea 486 checker inputs directly.
- Do not treat helper-local reachability/dominance queries as durable proof
  facts unless this runbook publishes explicit records/statuses.
- Keep `lir_producer_instruction_index` as a LIR producer-site coordinate.
- Existing untracked `review/*.md` files are transient and must remain
  untouched.

## Proof

Lifecycle activation validation:

```sh
git diff --check
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed
python3 scripts/plan_review_state.py show
```

Result: passed. `test_after.log` was absent and logs were out of scope for this
lifecycle-only delegation, so regression sanity used the unchanged canonical
`test_before.log` as both inputs (`328/328`). Hook-backed state now reports
Step 1, `Audit Collector Inputs And Matching Keys`.
