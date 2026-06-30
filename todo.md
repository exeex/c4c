Status: Active
Source Idea Path: ideas/open/470_branch_stack_load_policy_freshness.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Branch-Site Policy Freshness Inputs

# Current Packet

## Just Finished

Closed idea 469 as complete for prepared branch-stack-load record surface and
activated idea 470 for the next producer prerequisite: branch-site
`load_from_stack_slot` policy, freshness, and clobber-safety.

Representative residuals from 469:

| Row | Current record | Follow-up boundary |
| --- | --- | --- |
| `f.block_1` condition `%t2` | `unsupported_terminator`, `pointer_status=not_pointer` | Needs branch-site relationship/policy/freshness/clobber proof. |
| `f.block_1` lhs `%t1` | `unsupported_terminator`, `pointer_status=unknown` | Needs policy/freshness/clobber plus explicit pointer status. |
| `f.block_4` condition `%t8` | `unsupported_terminator`, `pointer_status=not_pointer` | Needs branch-site relationship/policy/freshness/clobber proof. |
| `f.block_4` lhs `%t7` | `unsupported_terminator`, `pointer_status=unknown` | Pointer-value/provenance remains separate. |
| `f.logic.end.14` condition `%t23` | `missing_policy` | Needs explicit branch-site load policy, freshness, and clobber safety. |
| `f.logic.end.14` lhs `%t22` | `missing_policy` | Select-result stack-destination remains separate. |

## Suggested Next

Execute Step 1 from `plan.md`: audit branch-site policy, freshness, and
clobber-safety inputs for the current prepared branch-stack-load records.

Suggested artifact directory:
`build/agent_state/470_step1_policy_freshness_audit/`.

## Watchouts

- Do not edit implementation files during Step 1.
- Do not implement RV64 branch-load emission in this producer plan.
- Do not accept unavailable `PreparedBranchStackLoadAuthority` records as
  target authority.
- Do not infer load authority from stack homes, offsets, object ids, raw BIR,
  block labels, function names, or testcase shape.
- Keep pointer-value/provenance and select-result stack-destination boundaries
  separate.
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
