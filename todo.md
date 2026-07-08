Status: Active
Source Idea Path: ideas/open/601_rv64_gcc_torture_1000_pass_recovery_umbrella.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Generate Ordered Follow-Up Ideas

# Current Packet

## Just Finished

Completed Step 4: Generate Ordered Follow-Up Ideas by creating ordered
follow-up source ideas `602` through `618` under `ideas/open/`. The queue
matches the Step 3 dependency order: BIR local-memory producer repairs first,
then BIR global initializer bootstrap, destination fan-in research,
prepared/global authority, RV64/global consumption, RV64/MIR consumers,
ABI/RV64 lowering, recent-architecture-close wiring tails, scalar compare
publication, and runtime mismatch ownership research.

## Suggested Next

Begin Step 5 in `plan.md`: write
`docs/rv64_gcc_torture_1000_pass_recovery/dependency_order_to_1000.md` using
the generated `ideas/open/602` through `ideas/open/618` queue. The document
should state the recommended activation order, producer-before-consumer
dependencies, first `1000+` route, deferred or quarantined families, and
research/discussion gates.

## Watchouts

- This umbrella is triage and follow-up generation only; do not edit
  implementation, harness, expectation, unsupported-marker, allowlist,
  runtime, timeout, or accounting behavior.
- The generated ideas intentionally split local-memory producer work,
  prepared/global authority, RV64/global consumption, RV64/MIR consumers,
  ABI/RV64 consumption, and recent-architecture wiring tails by owner.
- The `125` destination fan-in rows and `72` runtime mismatch rows plus `3` run
  timeouts are routed through research/ownership mapping before implementation.
- Deferred or quarantined lanes should stay out of the first `1000+` route
  unless later evidence changes their breadth or policy status.

## Proof

Lifecycle proof for Step 4:

```sh
test -f ideas/open/602_bir_local_memory_load_semantics.md &&
test -f ideas/open/618_runtime_mismatch_ownership_investigation.md &&
rg 'Owning Layer:|Prerequisites:|Estimated Evidence Breadth:|Proof Surface:|## Acceptance Criteria|## Reviewer Reject Signals' ideas/open/60{2..9}_*.md ideas/open/61{0..8}_*.md
```
