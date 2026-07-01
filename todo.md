Status: Active
Source Idea Path: ideas/open/476_semantic_instruction_result_frame_slot_materialization_facts.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Semantic Materialization Interval Inputs

# Current Packet

## Just Finished

Closed idea 475 as a routed/blocked population attempt. The source-fact carrier
can represent real records, but current prepared evidence lacks lower-level
semantic instruction-result frame-slot materialization/write records and
path/no-clobber interval facts.

The new active idea 476 owns that lower-level producer. Source-fact population,
downstream branch-stack-load authority, and RV64 branch-load consumption remain
blocked.

## Suggested Next

Execute Step 1 from `plan.md`: audit semantic instruction-result
materialization/write and interval no-clobber inputs for `%t23` slot `#21`.

Suggested artifact directory:
`build/agent_state/476_step1_semantic_materialization_interval_audit/`.

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
