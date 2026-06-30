Status: Active
Source Idea Path: ideas/open/463_select_edge_ule_source_producer_rematerialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Select-Edge Source-Producer Rejection

# Current Packet

## Just Finished

Closed idea 462 as a completed negative route-classification slice and
activated idea 463 for the focused `%t46 -> %t50` select-edge source-producer
rematerialization owner.

The rejected plain-copy route:

| Field | Value |
| --- | --- |
| Event | `pre_terminator_copies`, `main`, `block_index=10`, `block_label=logic.rhs.end.40` |
| Edge | `logic.rhs.end.40 -> logic.end.41` |
| Authority | `out_of_ssa_parallel_copy` |
| Move | `%t46` / `from_value_id=20` to `%t50` / `to_value_id=21`, `destination_storage=register` |
| Source producer | `%t46 = bir.ule ptr %t42, %t45` in successor block `logic.end.41` |
| Carrier facts | `%t50.phi.sel0` / `%t50.phi.sel1` |
| Rejection | Plain predecessor-edge GPR copy is unsound because `%t46` is not predecessor-available from value homes alone. |

## Suggested Next

Execute Step 1 from `plan.md`: audit why the existing select-edge
source-producer consumer rejects or misses `%t46 = bir.ule ptr %t42, %t45`
for `%t50` on edge `logic.rhs.end.40 -> logic.end.41`.

Suggested artifact directory:
`build/agent_state/463_step1_select_edge_ule_source_producer_audit/`.

## Watchouts

- Do not edit implementation files during Step 1.
- Do not implement a plain `%t46 -> %t50` copy or same-register no-op.
- Do not infer predecessor availability from prepared value homes alone.
- Do not add generic stack-to-register, register-to-register, or all-purpose
  move-bundle support.
- Do not consume `load_from_stack_slot missing_stack_freshness`.
- Do not reopen ideas 456, 458, 459, 460, 461, or 462 without new
  coordinate-bearing evidence that their exact route owns the first failure.
- Do not match on testcase name, function name, block label, value ids alone,
  raw BIR shape, or prepared dump order.
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
