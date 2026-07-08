Status: Active
Source Idea Path: ideas/open/601_rv64_gcc_torture_1000_pass_recovery_umbrella.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Write the Dependency Order to 1000

# Current Packet

## Just Finished

Completed Step 5: Write the Dependency Order to 1000 by creating
`docs/rv64_gcc_torture_1000_pass_recovery/dependency_order_to_1000.md`.
The document states the recommended activation order for ideas `602` through
`618`, producer-before-consumer gates, which ideas must run before RV64
consumers, the first `1000+` route, deferred and quarantined lanes, and the
destination fan-in, runtime mismatch, and pointer/address research gates.

## Suggested Next

Begin Step 6 in `plan.md`: assemble
`docs/rv64_gcc_torture_1000_pass_recovery/index.md` with links to the Step
1-5 handoff documents and generated follow-up ideas, plus closure notes for
evidence, bucket method, generated queue, high-yield route, architecture weak
points, deferred families, stale-count guidance, and recommended next
lifecycle activation.

## Watchouts

- This umbrella is triage and follow-up generation only; do not edit
  implementation, harness, expectation, unsupported-marker, allowlist,
  runtime, timeout, or accounting behavior.
- The generated ideas intentionally split local-memory producer work,
  prepared/global authority, RV64/global consumption, RV64/MIR consumers,
  ABI/RV64 consumption, and recent-architecture wiring tails by owner.
- The dependency order keeps the `125` destination fan-in rows and `72`
  runtime mismatch rows plus `3` run timeouts behind research/ownership gates
  before implementation.
- Deferred or quarantined lanes should stay out of the first `1000+` route
  unless later evidence changes their breadth or policy status.
- Step 6 should verify all handoff docs still agree on the current `470/1467`
  evidence and do not reuse stale `349/1467`, `425/1467`, or `438/1467`
  counts as current.

## Proof

Documentation proof for Step 5:

```sh
test -f docs/rv64_gcc_torture_1000_pass_recovery/dependency_order_to_1000.md && rg '602|618|producer-before-consumer|1000\+|deferred|quarantined|destination fan-in|runtime mismatch|activation order' docs/rv64_gcc_torture_1000_pass_recovery/dependency_order_to_1000.md
```
