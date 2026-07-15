# Current Packet

Status: Active
Source Idea Path: ideas/open/802_lir_switch_selector_type_reference_verifier.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Diagnose and repair switch selector type-reference verification

## Just Finished

- Lifecycle switch from 801 Step 2: Step 1 remains accepted in `827dae5bd3`;
  the current 801 compatibility repair remains preserved, uncommitted, and
  unaccepted. Its focused proof now reaches the 802 selector gate.

## Suggested Next

- Step 1: assess only the parked `LirSwitch.selector_type_ref` verifier/test
  hunk, tracing the ownership/type-comparison seam introduced by `a6c013ed0`.

## Watchouts

- Do not weaken the selector verifier or use display text as type authority.
  Do not modify, accept, or broaden 801's anonymous-layout/direct-complex-call
  repair, and do not evaluate or claim the unrelated 754 aggregate-use hunk.
  Return to 801 only after this blocker has accepted proof.

## Proof

- Gate reached: fresh `cmake --build --preset default` then
  `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure`
  passed with the parked 802 hunk; temporarily rolling back that hunk made the
  same test fail at the selector assertion, and it was restored. This is not
  802 acceptance proof; Step 1 must run its own fresh focused positive and
  malformed verifier proof.
