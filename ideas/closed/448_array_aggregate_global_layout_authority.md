# Array/Aggregate Global Layout Authority

Status: Closed
Type: Producer/prepared authority idea
Parent: `ideas/closed/446_global_memory_layout_authority_publication.md`
Source Evidence: `build/agent_state/446_step4_residual_disposition/`
Close Evidence: `build/agent_state/448_step4_residual_disposition/`
Owning Layer: BIR/prepared global memory layout authority before RV64 lowering

## Goal

Publish explicit array/aggregate global layout authority for prepared
global-symbol memory accesses such as `930930-1 @mem+792`, without RV64
target-side inference.

## Completion Notes

Closed as complete for integer-array global layout authority.

- Step 3 published producer/prepared integer-array global layout authority.
- Step 4 re-probed representatives and found `930930-1 @mem+792` now carries
  `layout_authority=byte_storage_aggregate` with
  `range_verdict=proven_in_bounds`.
- Idea 439 global-memory publication predicates can consume the accepted row
  without RV64 target-side inference.
- `20041112-1` scalar global rows remain owned by closed idea 446 and have
  separate direct-global/terminator/immediate-source residual owners.
- `20000622-1` has no global-symbol memory row and remains outside this source
  idea.

## Residual Disposition

The remaining object-route failures are outside idea 448:

- frame-slot call-argument publication;
- pointer-value and local store publication;
- unsupported destination-storage publication;
- generic RV64 instruction fragments;
- direct-global return/select-chain and terminator publication;
- immediate global-store source encoding, tracked by
  `ideas/open/447_immediate_global_store_source_encoding.md`.

Structured aggregate layout authority is not split here because Step 4 did not
identify a representative/source packet beyond integer arrays. Create a fresh
source idea only when concrete structured aggregate evidence justifies it.

## Close Validation

Close gate used the existing canonical backend logs:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Result: PASS, with `test_before.log` and `test_after.log` both reporting
`327/327` passing tests.

`git diff --check` also passed.

## Reviewer Reject Signals

- Reject reopening this closed idea to infer non-scalar global layout in RV64
  from raw BIR, symbol spelling, object labels, representative filenames, or
  dump shape.
- Reject claiming `ByteStorageAggregate` integer-array authority covers
  arbitrary structured aggregate byte lanes, pointer arrays, strings,
  extern/TLS globals, or raw-only unresolved symbols without a new source
  contract.
- Reject weakening idea 439 publication predicates so unknown layout authority
  becomes target-consumable.
- Reject expectation rewrites, unsupported-marker downgrades, allowlists, or
  pass/fail accounting changes claimed as array/aggregate layout progress.
- Reject baseline/log churn that accepts or rewrites `test_baseline.new.log`.
