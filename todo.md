Status: Active
Source Idea Path: ideas/open/aarch64-codegen-01-calls-dispatch-bridge-helper-absorption.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Bridge Helpers and Durable Owners

# Current Packet

## Just Finished

Lifecycle activation created the active runbook and execution-state skeleton
for Step 1.

## Suggested Next

Execute Step 1: inspect `calls_dispatch_bridge.*`, classify bridge helpers,
identify the first bounded helper family to absorb, and record the selected
receiving owner before making implementation edits.

## Watchouts

- Keep the work behavior-preserving.
- Do not broaden into `dispatch.cpp`, whole-call-family cleanup, phase
  extraction, ABI redesign, or expectation rewrites.
- Leave helpers in the bridge when absorption would mix responsibilities or
  change call behavior.

## Proof

Not run. Activation is lifecycle-only.
