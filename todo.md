Status: Active
Source Idea Path: ideas/open/473_branch_site_stack_slot_materialization_no_clobber_source.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Materialization No-Clobber Source Inputs

# Current Packet

## Just Finished

Closed idea 472 as a blocked/negative certificate-producer result. It proved
that downstream certificate/authority surfaces can consume facts, but current
prepared evidence lacks exact source value plus frame-slot materialization,
path validity, same-slot write exclusion, call/helper/inline-asm safety, and
publication/move/parallel-copy non-clobber source facts.

The new active idea 473 owns only those lower-level source facts. Downstream
branch-stack-load availability and RV64 branch-load consumption remain blocked.

## Suggested Next

Execute Step 1 from `plan.md`: audit materialization/current-value and
no-clobber source inputs for `%t23` slot `#21` and adjacent boundary rows.

Suggested artifact directory:
`build/agent_state/473_step1_materialization_no_clobber_audit/`.

## Watchouts

- Do not edit implementation files during Step 1.
- Do not implement RV64 branch-load emission in this producer plan.
- Do not directly populate available branch-stack-load authority records.
- Do not infer source facts from stack homes, offsets, object ids, raw BIR,
  value names, block labels, function names, testcase names, or dump order.
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
