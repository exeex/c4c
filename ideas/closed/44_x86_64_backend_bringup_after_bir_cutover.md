# x86_64 Backend Bring-Up After BIR Cutover

Status: Open
Last Updated: 2026-04-13

## Why This Idea

The post-BIR x86_64 lane no longer has its biggest problem in missing isolated
recoveries. The higher-leverage problem is that shared lowering still contains
too much legacy matcher debt. Execution drifted into adding more testcase-
shaped seams instead of growing reusable lowering families.

Representative debt includes:

- exact rendered-IR matching
- `try_lower_minimal_*` seams keyed to narrow source shapes
- duplicated aggregate and memory recognizers split by testcase flavor
- dispatcher pressure in `try_lower_to_bir_with_options(...)`
- continued fallback dependence on legacy lowering paths

Continuing to add more x86_64 recoveries on top of that surface would improve
pass count short-term but make the shared lowering tree harder to maintain and
harder to generalize.

## Goal

Turn this lane from testcase-shaped x86_64 recovery into family-based cleanup
of the remaining shared-BIR legacy matcher surface.

Concretely:

- shrink the `legacy-lowering` surface
- replace brittle exact-text seams with structured matching
- consolidate adjacent matcher families into reusable helpers
- reduce dispatcher pressure before resuming broader x86_64 recovery

## Non-Goals

- do not revive legacy backend IR
- do not restore LLVM asm rescue paths
- do not widen into a whole multi-target lowering rewrite
- do not absorb unrelated parser, runtime, or cross-target failures
- do not treat "add the next matcher" as the default execution mode

## Durable Constraints

- clean up one semantic family at a time, not one testcase at a time
- prefer structured matching over rendered-text matching
- preserve already-landed x86_64 wins while shrinking matcher debt
- separate native/runtime/toolchain issues into their own lifecycle items when
  they are not direct blockers to the active cleanup family

## Matcher Families

The remaining debt should be handled as a small number of families:

- global constant aggregate compare ladders
- local aggregate and local-slot compare ladders
- pointer / local-array address and alias families
- string / char compare ladders
- helper-call and bounded call seams

Future activations should name one family explicitly instead of re-entering
general "bring up the next failing x86_64 case" mode.

## Current Durable State

- this idea is parked cleanup work, not the active translated-code ownership
  lane
- the right abstraction level is family cleanup, not per-test recovery
- the next activation should start by inventorying and ranking the remaining
  legacy families, then choose the smallest high-leverage family slice

## Reactivation Notes

When this idea becomes active again:

- inventory the remaining matcher surface by family and file
- choose one bounded family slice
- replace brittle seams with structured or family-level ownership
- prove the cleanup with narrow regressions that survive naming/rendering
  changes
- resume new x86_64 failing-case recovery only after the matcher surface has
  clearly shrunk
