Status: Active
Source Idea Path: ideas/open/467_unsupported_carrier_alias_planner_rejection.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Unsupported Carrier Alias Rejection

# Current Packet

## Just Finished

Closed idea 466 as complete evidence/probe classification and activated idea
467 for the precise `unsupported_carrier_alias` planner/producer rejection.

The representative row is visible but unavailable:

| Field | Value |
| --- | --- |
| Representative | `20010329-1` |
| Status | `unsupported_carrier_alias` |
| Edge | `logic.rhs.end.40 -> logic.end.41` |
| Source / destination | `%t46` / `source_value_id=20` to `%t50` / `destination_value_id=21` |
| Source producer | `binary`, block `logic.end.41`, instruction `1` |
| Candidate count | `carrier_alias_candidates=2` |
| Accepted aliases | `carrier_aliases=0` |
| Use closure | `source_use_closure=no` |
| Boundary | Evidence row is diagnostic only, not RV64 authority. |

## Suggested Next

Execute Step 1 from `plan.md`: audit why the prepared carrier-alias planner
rejects two carrier candidates as `unsupported_carrier_alias` before authority
publication.

Suggested artifact directory:
`build/agent_state/467_step1_unsupported_carrier_alias_audit/`.

## Watchouts

- Do not edit implementation files during Step 1.
- Do not treat unavailable evidence rows as authority.
- Do not make RV64 ULE rematerialization changes until an available authority
  record exists or a later packet proves an RV64 matcher mismatch against one.
- Do not infer aliases from `%*.phi.sel*` spelling, raw select shape, value
  ids, block labels, function names, testcase names, or dump order.
- Do not implement plain `%t46 -> %t50` copies or same-register no-ops.
- Do not add generic stack-to-register, register-to-register, or all-purpose
  move-bundle support.
- Do not consume `load_from_stack_slot missing_stack_freshness`.
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
