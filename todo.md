Status: Active
Source Idea Path: ideas/open/472_branch_site_stack_slot_current_value_no_clobber_certificate.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Current-Value No-Clobber Certificate Inputs

# Current Packet

## Just Finished

Closed idea 471 as a negative producer-feasibility result. It proved the
carrier/planner surface is adequate, but real representative rows still lack a
producer-owned certificate proving current-value source identity and
no-clobber safety for the branch-site stack slot.

The new active idea 472 owns only that certificate producer prerequisite. RV64
branch-load consumption remains blocked until available records exist.

## Suggested Next

Execute Step 1 from `plan.md`: audit current-value and no-clobber certificate
inputs for the representative branch-stack-load records.

Suggested artifact directory:
`build/agent_state/472_step1_current_value_no_clobber_audit/`.

## Watchouts

- Do not edit implementation files during Step 1.
- Do not implement RV64 branch-load emission in this producer plan.
- Do not accept unavailable `PreparedBranchStackLoadAuthority` records as
  target authority.
- Do not infer current value or no-clobber safety from stack homes, offsets,
  object ids, raw BIR, block labels, function names, testcase names, or dump
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
