Status: Active
Source Idea Path: ideas/open/474_prepared_frame_slot_materialization_no_clobber_facts.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Prepared Source-Fact Carrier Requirements

# Current Packet

## Just Finished

Closed idea 473 as a routed/blocked source-fact initiative. It proved current
prepared/prealloc data lacks an independent carrier/producer for frame-slot
materialization/write events, path/dominance validity, same-slot write
exclusion, call/helper/inline-asm safety, and publication/move/parallel-copy
non-clobber classification.

The new active idea 474 owns that lower-level prepared source-fact surface.
Downstream branch-stack-load availability and RV64 branch-load consumption
remain blocked.

## Suggested Next

Execute Step 1 from `plan.md`: audit prepared source-fact carrier requirements
for frame-slot materialization/write and no-clobber facts.

Suggested artifact directory:
`build/agent_state/474_step1_source_fact_carrier_audit/`.

## Watchouts

- Do not edit implementation files during Step 1.
- Do not implement RV64 branch-load emission in this producer plan.
- Do not directly populate available branch-stack-load authority records.
- Do not infer source facts from stack homes/storage, offsets, object ids, raw
  BIR, value names, block labels, function names, testcase names, or dump
  order.
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
