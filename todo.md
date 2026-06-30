Status: Active
Source Idea Path: ideas/open/471_branch_site_stack_slot_freshness_clobber_safety_metadata.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Branch-Site Freshness And Clobber Inputs

# Current Packet

## Just Finished

Closed idea 470 as split/blocker documentation after Step 4 concluded the
carrier and unavailable statuses are adequate, but available records still lack
producer-owned branch-site stack-slot freshness and clobber-safety evidence.

The new active idea 471 owns that precise producer prerequisite. RV64
branch-load consumption remains blocked until available records are backed by
explicit freshness and clobber-safety facts.

## Suggested Next

Execute Step 1 from `plan.md`: audit branch-site freshness and clobber inputs
for the current prepared branch-stack-load records.

Suggested artifact directory:
`build/agent_state/471_step1_freshness_clobber_audit/`.

## Watchouts

- Do not edit implementation files during Step 1.
- Do not implement RV64 branch-load emission in this producer plan.
- Do not accept unavailable `PreparedBranchStackLoadAuthority` records as
  target authority.
- Do not infer freshness or clobber safety from stack homes, offsets, object
  ids, raw BIR, block labels, function names, testcase names, or dump order.
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
