Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Refresh Current First Bad Fact

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from
`ideas/open/326_aarch64_variadic_hfa_floating_residual.md`.

## Suggested Next

Start `plan.md` Step 1 by refreshing the current `00204.c` generated-code
first bad fact. Stop for lifecycle handoff if the current owner is outside
HFA/floating or composite variadic call-boundary scope.

## Watchouts

- Do not continue from stale historical HFA notes without fresh artifacts.
- Do not reopen stdarg format traversal, MOVI zero-extension, fixed-formal
  entry, byval lane placement, or other split owners unless fresh first-bad-fact
  evidence proves this source idea owns the current failure.
- Do not special-case `00204.c`, `myprintf`, one HFA lane, one register, one
  stack offset, or one emitted instruction sequence.

## Proof

Lifecycle-only activation; no implementation validation run.
