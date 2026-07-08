Status: Active
Source Idea Path: ideas/open/601_rv64_gcc_torture_1000_pass_recovery_umbrella.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Draft the High-Yield Follow-Up Plan

# Current Packet

## Just Finished

Completed Step 3: Draft the High-Yield Follow-Up Plan by writing
`docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md`. The
plan ranks follow-up families for generation, deferral, or quarantine; explains
expected breadth from the Step 2 bucket map; identifies research candidates for
destination fan-in, runtime mismatch ownership, and pointer/address boundaries;
and records why the high-yield candidates are not testcase-overfit.

## Suggested Next

Begin Step 4 in `plan.md`: generate ordered follow-up ideas under
`ideas/open/` from the ranked queue in
`docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md`, keeping
each idea single-owner with prerequisites, estimated breadth, proof surface,
acceptance criteria, and reviewer reject signals.

## Watchouts

- This umbrella is triage and follow-up generation only; do not edit
  implementation, harness, expectation, unsupported-marker, allowlist,
  runtime, timeout, or accounting behavior.
- Step 4 should split global data by prepared authority versus RV64 consumer
  and split RV64/MIR consumer ideas by move-bundle, terminator, and instruction
  fragment families rather than opening mixed-owner implementation routes.
- The `125` destination fan-in rows and `72` runtime mismatch rows are
  high-count, but the Step 3 plan routes them through research or ownership
  mapping before implementation.
- Deferred or quarantined lanes should stay out of the first `1000+` route
  unless later evidence changes their breadth or policy status.

## Proof

Delegated proof passed and was saved to `test_after.log`:

```sh
test -f docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md && rg 'BIR local-memory|RV64/MIR|destination fan-in|global data|ABI|runtime mismatch|defer|quarantine|not testcase-overfit|1000' docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md
```
