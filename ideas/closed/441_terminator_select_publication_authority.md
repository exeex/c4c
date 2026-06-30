# Terminator Select Publication Authority

Status: Closed
Type: Terminator/select publication idea
Parent: `ideas/closed/433_rv64_global_select_pointer_memory_residuals.md`
Source Evidence: `build/agent_state/433_step4_residual_disposition/`
Close Evidence: `build/agent_state/441_step4_residual_disposition/`
Owning Layer: Terminator and select publication facts before RV64 lowering

## Goal

Establish explicit publication authority for terminator/select residuals so
RV64 control-flow lowering can consume prepared facts without reconstructing
them from raw BIR.

## Completion Notes

Closed as complete for the selected fused pointer `eq/ne` branch publication
route.

- Step 2 selected a bounded fused pointer equality/inequality branch
  publication contract.
- Step 3 implemented the prepared-fact-driven route for
  `PreparedBranchConditionKind::FusedCompare` with pointer `Eq`/`Ne`
  predicates and GPR-compatible homes.
- Step 4 re-probed representatives and found `20041112-1` now emits RV64
  object successfully; the former `bar.entry` fused pointer `ne` branch no
  longer owns `unsupported_terminator_fragment`.
- Focused coverage preserves fail-closed behavior for pointer relational
  predicates, missing branch conditions, missing pointer operand homes,
  mismatched labels, non-null pointer immediates, and non-pointer compare
  types in the pointer publication predicate.

## Residual Disposition

No remaining residual is the same fused pointer `eq/ne` branch authority gap.

Durable follow-up ideas:

- `ideas/open/449_pointer_relational_branch_publication.md` for pointer
  relational fused compares such as `uge ptr` and `ult ptr`;
- `ideas/open/450_select_result_branch_publication.md` for
  `root_is_select=yes` select-result publication into branch conditions such
  as `ne i32 <select>, 0`.

Still out of scope or first-owned elsewhere:

- direct-global return authority, closed by idea 440;
- direct-global select-chain/global store-source facts that no longer block
  `20041112-1`;
- pointer-value memory, local/global store-source publication, unsupported
  instruction fragments, stack-slot/unsupported destination storage;
- expectation rewrites, allowlists, pass/fail accounting, and accepting or
  modifying `test_baseline.new.log`.

## Close Validation

Supervisor-provided validation says the Step 4 backend proof and regression
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

- Reject reopening this closed idea to infer fused pointer branch operands,
  targets, or homes from raw BIR shape, block order, filenames, function names,
  testcase names, or one prepared dump layout.
- Reject treating pointer relational predicates (`ult`, `ule`, `ugt`, `uge`,
  signed variants) as covered by the closed pointer `eq/ne` route.
- Reject treating `root_is_select=yes` rows as branch-publication authority
  without a separate select-result contract.
- Reject expectation rewrites, unsupported-marker downgrades, allowlists, or
  pass/fail accounting changes claimed as terminator/select progress.
- Reject baseline/log churn that accepts or rewrites `test_baseline.new.log`.
