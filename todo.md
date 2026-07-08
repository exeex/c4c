Status: Active
Source Idea Path: ideas/open/601_rv64_gcc_torture_1000_pass_recovery_umbrella.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Build the Failure Bucket Map

# Current Packet

## Just Finished

Completed Step 2: Build the Failure Bucket Map by classifying the current
`997` RV64 gcc_torture backend failures by first owner and capability family in
`docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`. The map
accounts for all current failures, separates BIR semantic producer,
prepared/prealloc authority, RV64/MIR consumer, runtime, unsupported, ABI,
architecture, and policy lanes, calls out recent-architecture-close buckets
from ideas `587` through `600`, and names architecture weak points needing
research or discussion before implementation.

## Suggested Next

Begin Step 3 in `plan.md`: draft the high-yield follow-up plan in
`docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md`,
ranking which classified families should generate follow-up ideas, be deferred,
or be quarantined.

## Watchouts

- This umbrella is triage and follow-up generation only; do not edit
  implementation, harness, expectation, unsupported-marker, allowlist,
  runtime, timeout, or accounting behavior.
- The largest first-owner buckets are BIR semantic producer (`311`), RV64/MIR
  consumer (`252`), prepared/prealloc authority (`146`), runtime (`72`),
  ABI/RV64 consumer (`60`), prepared/RV64 authority (`48`), and combined global
  data (`70` across prepared/global authority and RV64/global consumer).
- The `125` non-parallel multi-source stack-destination rows are high-count but
  should be treated as a destination fan-in architecture question, not as a
  simple extension of source-freshness authority.
- Runtime abort/segfault rows reached object emission, but exit symptoms alone
  do not identify first implementation owner.

## Proof

Delegated proof passed and was saved to `test_after.log`:

```sh
test -f docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md && rg '997|first owner|BIR|prepared|RV64|runtime|unsupported|ABI|architecture' docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md
```
