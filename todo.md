Status: Active
Source Idea Path: ideas/open/481_semantic_result_frame_slot_materialization_point_producer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Materialization Point Inputs

# Current Packet

## Just Finished

Closed idea 480 as a routed blocker. Semantic write-event production did not
complete because current prepared/prealloc evidence lacks an explicit
materialization point for `%t23 = bir.ne i32 %t22, 0` into slot `#21`.

The new active idea 481 owns that explicit semantic result frame-slot
materialization-point producer. Write-event consumption, event-authority
consumption, interval population, source-fact population, branch-stack-load
authority, and RV64 branch-load consumption remain blocked.

## Suggested Next

Execute Step 1 from `plan.md`: audit materialization-point inputs for `%t23`
slot `#21` and adjacent protected boundary rows.

Suggested artifact directory:
`build/agent_state/481_step1_materialization_point_audit/`.

## Watchouts

- Do not edit implementation files during Step 1.
- Do not implement RV64 branch-load emission in this producer plan.
- Do not directly populate write-event, event-authority, semantic interval,
  source-fact, or branch-stack-load authority records.
- Do not reuse `%t22 -> %t23` stack moves as semantic materialization evidence
  when `authority=none`.
- Do not infer materialization points from raw BIR, branch conditions, stack
  homes, storage, offsets, object ids, value names, function names, testcase
  names, or dump order.
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
