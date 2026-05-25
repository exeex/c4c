Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Local-Frame Publication Authority Leak

# Current Packet

## Just Finished

Step 5 closure review rejected closure for the aggregate-address publication
checkpoint and regenerated the active runbook around the surviving local-frame
publication authority leak.

- Re-scan found no retained `arg_abi` or `arg_types` decision reads in the
  aggregate-address publication path in `calls_dispatch_bridge.cpp`.
- Re-scan found surviving retained `CallInst::arg_types` and
  `CallInst::arg_abi` decision reads in `calls_argument_sources.cpp`.
- The source idea remains open because local-frame address publication still
  rederives pointer/byval eligibility from retained call metadata.
- Close-time regression guard was not run because closure was rejected before
  the close gate.

## Suggested Next

Execute Step 1 by selecting one local-frame publication authority leak in
`calls_argument_sources.cpp`, mapping it to an existing prepared fact or a
precise missing-prepared-fact blocker, and recording the focused proof scope.

## Watchouts

- Do not work on `ideas/open/03_dispatch_responsibility_reduction.md`.
- Do not weaken tests or mark a nearby publication case unsupported to claim
  progress.
- If the prepared fact needed for local-frame publication does not exist, stop
  with that blocker in `todo.md` instead of reconstructing the decision from
  `CallInst`.

## Proof

Lifecycle-only closure review. No build or test command was run.
