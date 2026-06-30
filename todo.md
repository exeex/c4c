Status: Active
Source Idea Path: ideas/open/464_select_carrier_alias_metadata.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Duplicate Carrier Alias Metadata Gap

# Current Packet

## Just Finished

Closed idea 463 as a completed negative route-classification slice and
activated idea 464 for the prepared carrier-alias metadata prerequisite.

The rejected RV64 route:

| Field | Value |
| --- | --- |
| Candidate rematerialization | `%t46 = bir.ule ptr %t42, %t45` for `%t46 -> %t50` |
| Edge | `logic.rhs.end.40 -> logic.end.41` |
| Duplicate carriers | `%t50.phi.sel0` / `%t50.phi.sel1` |
| Missing fact | Durable prepared metadata proving both carriers are carrier-only aliases of final `%t50` |
| First owner | Prepared control-flow/publication metadata |

## Suggested Next

Execute Step 1 from `plan.md`: audit prepared control-flow, prepared object
traversal, select-chain lookup, and publication-plan records for the duplicate
carrier shape and identify the first bounded metadata packet.

Suggested artifact directory:
`build/agent_state/464_step1_duplicate_carrier_alias_audit/`.

## Watchouts

- Do not edit implementation files during Step 1.
- Do not make RV64 ULE rematerialization or target consumer changes before the
  metadata exists.
- Do not infer duplicate-carrier authority from `%*.phi.sel*` spelling, raw
  select shape, value ids, block labels, function names, testcase names, or
  dump order.
- Do not implement a plain `%t46 -> %t50` copy or same-register no-op.
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
