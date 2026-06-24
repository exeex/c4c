# Current Packet

Status: Active
Source Idea Path: ideas/open/334_object_route_scan_and_default_readiness.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Resume Scan Baseline And Candidate Map

## Just Finished

- Closed child 336 after its final handoff proof showed the first scalar
  object-route scan baseline passing 28/28.
- Resumed idea 334 from the now-green scalar scan baseline.

## Suggested Next

- Inspect the current restored object-route selectors and choose the next safe
  object-route scan expansion candidates for Step 2.

## Watchouts

- Keep asm-route coverage intact.
- Do not add expected-failure labels for cases that should be supported.
- Keep RV64 globals/data, broader pointer/array/aggregate cases, broader
  AArch64 memory/frame/call shapes, AArch64 object runtime, x86 object output,
  object stdout, c-testsuite object defaults, and semantic-BIR object mode out
  of the first resumed scan packet unless the plan is explicitly revised.

## Proof

- Close proof for child 336 used matching 28-test `test_before.log` and
  `test_after.log`; non-decreasing regression guard passed.
