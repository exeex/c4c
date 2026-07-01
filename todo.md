Status: Active
Source Idea Path: ideas/open/478_semantic_instruction_result_write_event_carrier.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Semantic Write Event Carrier Inputs

# Current Packet

## Just Finished

Closed idea 477 as a routed blocker. Real semantic fact population did not
complete because no authoritative prepared event carrier proves `%t23 = ne i32
%t22, 0` was written or materialized into slot `#21`.

The new active idea 478 owns that semantic write/materialization event carrier.
Interval population, source-fact population, branch-stack-load authority, and
RV64 branch-load consumption remain blocked.

## Suggested Next

Execute Step 1 from `plan.md`: audit semantic write-event carrier inputs for
`%t23` slot `#21` and adjacent protected boundary rows.

Suggested artifact directory:
`build/agent_state/478_step1_semantic_write_event_carrier_audit/`.

## Watchouts

- Do not edit implementation files during Step 1.
- Do not implement RV64 branch-load emission in this producer plan.
- Do not directly populate semantic interval, source-fact, or
  branch-stack-load authority records.
- Do not reuse `%t22 -> %t23` stack moves as semantic materialization evidence
  when `authority=none`.
- Do not infer event authority from raw BIR, stack homes, storage, offsets,
  object ids, value names, function names, testcase names, or dump order.
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
