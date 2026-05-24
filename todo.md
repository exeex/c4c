Status: Active
Source Idea Path: ideas/open/aarch64-codegen-03-alu-fallback-operand-phase-extraction.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Fallback Operand Ownership

# Current Packet

## Just Finished

No executor packet has run yet. Lifecycle activation created the active
runbook and execution scratchpad for Step 1.

## Suggested Next

Delegate Step 1 to inspect the fallback operand helper group around
`make_scalar_fallback_operand`, identify helpers that must stay outside the
extraction, and record the selected phase-local boundary.

## Watchouts

- Keep the slice behavior-preserving.
- Do not widen `alu.hpp` with private fallback concepts.
- Stop for plan review if control-publication behavior must be touched.
- Reject expectation downgrades, unsupported-test conversions, and
  testcase-shaped shortcuts.

## Proof

Lifecycle-only activation. `git diff --check` should be run before handoff.
