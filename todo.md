Status: Active
Source Idea Path: ideas/open/480_semantic_instruction_result_frame_slot_write_event_producer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Semantic Write Event Producer Inputs

# Current Packet

## Just Finished

Closed idea 479 as a routed blocker. Real event-authority population did not
complete because no lower-level prepared producer records semantic
instruction-result materialization into slot `#21` for `%t23 = bir.ne i32
%t22, 0`.

The new active idea 480 owns that lower-level semantic write/materialization
event producer. Event-authority consumption, interval population, source-fact
population, branch-stack-load authority, and RV64 branch-load consumption
remain blocked.

## Suggested Next

Execute Step 1 from `plan.md`: audit semantic write-event producer inputs for
`%t23` slot `#21` and adjacent protected boundary rows.

Suggested artifact directory:
`build/agent_state/480_step1_semantic_write_event_producer_audit/`.

## Watchouts

- Do not edit implementation files during Step 1.
- Do not implement RV64 branch-load emission in this producer plan.
- Do not directly populate event-authority, semantic interval, source-fact, or
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
