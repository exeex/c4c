# Global Memory Layout Authority Publication

Status: Closed
Type: Producer/prepared authority idea
Parent: `ideas/closed/439_store_source_global_memory_publication_authority.md`
Source Evidence: `build/agent_state/439_step4_residual_disposition/`
Close Evidence: `build/agent_state/446_step4_residual_disposition/`
Owning Layer: BIR/prepared global memory layout authority before RV64 lowering

## Goal

Publish explicit global memory layout authority for prepared global-symbol
accesses so global load/store publication can become target-consumable without
RV64 inference.

## Completion Notes

Closed as complete for scalar global layout authority.

- Step 3 published scalar global layout authority for accepted scalar global
  records.
- Step 4 re-probed representatives and found `20041112-1 @global+0` rows now
  carry `layout_authority=scalar_layout` with
  `range_verdict=proven_in_bounds`.
- Idea 439 publication predicates can consume those scalar global rows without
  RV64 target-side inference.
- `20000622-1` has no global-symbol memory rows and remains outside this
  source idea.

## Residual Disposition

`930930-1 @mem+792` remains correctly fail-closed with
`layout_authority=unknown` despite a proven range verdict. That row is not a
scalar-global authority defect; it needs separate array/aggregate global
layout authority if pursued.

Durable follow-up:

- `ideas/open/448_array_aggregate_global_layout_authority.md`

Still out of scope:

- immediate global-store source encoding, tracked by
  `ideas/open/447_immediate_global_store_source_encoding.md`;
- direct-global return/select-chain authority;
- terminator/select publication;
- pointer-value memory and local/frame-slot residuals;
- accepting or modifying `test_baseline.new.log`.

## Close Validation

Close gate used the existing canonical backend logs:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Result: PASS, with `test_before.log` and `test_after.log` both reporting
`327/327` passing tests.

`git diff --check` also passed.

## Reviewer Reject Signals

- Reject reopening this closed scalar idea to infer non-scalar global layout in
  RV64 from raw BIR, symbol spelling, object labels, representative filenames,
  or dump shape.
- Reject claims that `930930-1 @mem+792` is covered by scalar global layout
  authority while it still reports `layout_authority=unknown`.
- Reject weakening idea 439 publication predicates so unknown layout authority
  becomes target-consumable.
- Reject expectation rewrites, unsupported-marker downgrades, allowlists, or
  pass/fail accounting changes claimed as layout authority progress.
- Reject baseline/log churn that accepts or rewrites `test_baseline.new.log`.
