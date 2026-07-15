# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.38
Current Step Title: Receive the 825-authorized DirectScalar switch-selector parameter authority row

## Just Finished

- No executor packet has run on the resumed 734 route.

## Suggested Next

- Execute Step 7.38 only: receive closed 825's direct `LirSwitch.selector`
  DirectScalar parameter-authority tuple in typed Raw BIR and preserve its
  exact selector and selector-type coherence checks.

## Watchouts

- Do not touch preserved dirty Idea 821/822 work or reopen the accepted 734
  DirectPointer, DirectScalar binary-LHS/RHS, and ReturnValue rows.
- Do not reuse binary-LHS authority, materialize an `add`, or derive facts from
  names, signatures, rendered operands, diagnostics, or `monostate`.

## Proof

- Select the focused same-feature Raw-BIR receiver proof before implementation;
  require a fresh build and matching before/after regression guard for the
  chosen command.
