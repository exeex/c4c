# Direct-Global Return And Select-Chain Authority

Status: Closed
Type: Producer/consumer authority idea
Parent: `ideas/closed/433_rv64_global_select_pointer_memory_residuals.md`
Source Evidence: `build/agent_state/433_step4_residual_disposition/`
Close Evidence: `build/agent_state/440_step6_final_residual_disposition/`
Owning Layer: Direct-global return and select-chain prepared authority

## Goal

Define and consume explicit authority for direct-global return values and
direct-global select chains before RV64 lowering materializes them.

## Completion Notes

Closed as complete for the direct-global return authority route.

- Step 3 added the prepared direct-global return authority contract through
  `plan_prepared_direct_global_return_authority` and focused fail-closed
  coverage.
- Step 5 added the RV64 object-route consumer for explicit direct-global
  return authority and fail-closed tests for raw-only, missing/mismatched
  return moves, unsupported homes, unsupported destinations, and incoherent
  authority shapes.
- The transient reviewer report
  `review/440_step5_direct_global_return_consumer_review.md` found no
  blocking issues and judged the Step 5 route aligned with the source idea.
- Step 6 re-probed `20041112-1` and confirmed `foo.block_1` direct-global
  return authority remains complete and consumed through explicit prepared
  facts.

## Residual Disposition

No exact remaining direct-global return packet remains.

Direct-global select-chain candidate facts remain visible, but Step 6 did not
isolate an independent direct-global select-chain consumer failure. Fresh
evidence points the remaining `20041112-1` object-route failure at
`bar.entry`:

- `%t6 = bir.ne ptr %t2, %t5`;
- `bir.cond_br i32 %t6, block_4, block_5`;
- prepared `branch_condition entry kind=fused_compare`.

That residual belongs to
`ideas/open/441_terminator_select_publication_authority.md`, not more idea 440
work.

Still out of scope:

- generic terminator/select publication;
- store-source/global-memory publication and layout authority;
- pointer-value memory authority;
- local/frame-slot publication;
- accepting or modifying `test_baseline.new.log`.

## Close Validation

Supervisor-provided validation says the Step 6 backend proof and regression
guard passed before log roll-forward, with before and after both reporting
`327/327` passing tests.

Because `test_after.log` had already been rolled forward and was absent at
close review time, the plan-owner performed a read-only guard sanity check
against the rolled-forward canonical log:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed
```

Result: PASS, with `327/327` passing tests.

`git diff --check` also passed.

## Reviewer Reject Signals

- Reject reopening this closed idea to infer direct-global return handling from
  raw `bir.ret ptr @global`, symbol spelling, function names, testcase names,
  or one prepared dump layout.
- Reject claiming the remaining `bar.entry` fused pointer-compare conditional
  branch is direct-global return work; route it through terminator/select
  publication authority.
- Reject folding generic terminator/select lowering into this closed
  direct-global route.
- Reject expectation rewrites, unsupported-marker downgrades, allowlists, or
  pass/fail accounting changes claimed as direct-global authority progress.
- Reject baseline/log churn that accepts or rewrites `test_baseline.new.log`.
