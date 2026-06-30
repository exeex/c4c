Status: Active
Source Idea Path: ideas/open/465_rv64_ule_select_edge_rematerialization_consumer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Carrier-Alias Consumer Surface

# Current Packet

## Just Finished

Closed idea 464 as complete prepared carrier-alias metadata work and activated
idea 465 for the later RV64 ULE rematerialization consumer.

The new active plan may only consume explicit prepared authority:

| Field | Value |
| --- | --- |
| Authority records | `PreparedSelectCarrierAliasAuthorityRecords` |
| Source producer | `%t46 = bir.ule ptr %t42, %t45` |
| Selected source / destination | `%t46 -> %t50` |
| Duplicate carriers | `%t50.phi.sel0` / `%t50.phi.sel1` |
| Edge | `logic.rhs.end.40 -> logic.end.41` |
| Required boundary | No raw `.phi.sel` alias inference, no plain `%t46 -> %t50` copy, no stale stack-load consumption, no generic move support. |

## Suggested Next

Execute Step 1 from `plan.md`: audit how
`PreparedSelectCarrierAliasAuthorityRecords` are reachable from the RV64
object route, how they match the coordinate-bearing `pre_terminator_copies`
event, and whether `%t42` / `%t45` are target-consumable at the predecessor
edge.

Suggested artifact directory:
`build/agent_state/465_step1_carrier_alias_consumer_audit/`.

## Watchouts

- Do not edit implementation files during Step 1.
- Do not infer duplicate-carrier aliases from `%*.phi.sel*` spelling, raw
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
