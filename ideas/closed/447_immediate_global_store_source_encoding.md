# Immediate Global-Store Source Encoding

Status: Closed
Type: Producer/prepared publication idea
Parent: `ideas/closed/439_store_source_global_memory_publication_authority.md`
Source Evidence: `build/agent_state/439_step4_residual_disposition/`
Close Evidence: `build/agent_state/447_step4_residual_disposition/`
Owning Layer: BIR/prepared store-source publication before RV64 global stores

## Goal

Publish explicit immediate-source encoding for global stores so store-global
publication authority can distinguish immediate values from unknown sources.

## Completion Notes

Closed as complete for immediate global-store source encoding.

- Step 3 published explicit immediate source producer facts for the bounded
  immediate-valued global-store route.
- Step 4 re-probed `20041112-1 main.entry.0` and found
  `source_producer=immediate` for `bir.store_global @global, i32 1`.
- The destination row is coherent for this source idea:
  `layout_authority=scalar_layout` with
  `range_verdict=proven_in_bounds`.
- Existing fail-closed behavior for missing or unknown source facts was
  preserved by focused Step 3 coverage.

## Residual Disposition

No remaining in-scope immediate-source packet was identified.

Remaining representative residuals belong to other owners:

- non-immediate binary global stores with `source_producer=binary`;
- local/frame-slot store publication;
- direct-global/select-chain authority;
- RV64 `unsupported_terminator_fragment` target lowering.

Do not route those residuals through this closed immediate-source idea.

## Close Validation

Supervisor-provided validation says the Step 4 backend proof and supervisor
regression guard passed before log roll-forward, with before and after both
reporting `327/327` passing tests.

Because `test_after.log` had already been rolled forward and was absent at
close review time, the plan-owner performed a read-only guard sanity check
against the rolled-forward canonical log:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed
```

Result: PASS, with `327/327` passing tests.

`git diff --check` also passed.

## Reviewer Reject Signals

- Reject reopening this closed idea to infer immediate store values in RV64
  from raw BIR store shape, testcase names, block labels, symbol names, or one
  prepared dump layout.
- Reject weakening store-source authority so `source_producer=unknown` becomes
  target-consumable.
- Reject claiming non-immediate binary global stores, local/frame-slot stores,
  direct-global/select-chain rows, or terminator lowering are covered by this
  immediate-source closure.
- Reject expectation rewrites, unsupported-marker downgrades, allowlists, or
  pass/fail accounting changes claimed as source-encoding progress.
- Reject baseline/log churn that accepts or rewrites `test_baseline.new.log`.
