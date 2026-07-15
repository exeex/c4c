# Current Packet

Status: Active
Source Idea Path: ideas/open/802_lir_switch_selector_type_reference_verifier.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Diagnose and repair switch selector type-reference verification

## Just Finished

- Lifecycle switch from 801 Step 2: its accepted Step 1 is `827dae5bd3`; its
  current anonymous-layout/call repair remains uncommitted and unaccepted.

## Suggested Next

- Step 1: trace the `LirSwitch.selector_type_ref` ownership/type-comparison
  seam introduced by `a6c013ed0`, then repair the bounded verifier defect with
  nearby valid and malformed coverage.

## Watchouts

- Do not weaken the selector verifier or use display text as type authority.
  Do not modify, accept, or broaden 801's anonymous-layout/direct-complex-call
  repair. Return to 801 only after this blocker has accepted proof.

## Proof

- Fresh `cmake --build --preset default`, then focused nearby positive and
  malformed switch verifier coverage. The supervisor selects any wider proof
  needed for the shared verifier seam before accepting the blocker.
