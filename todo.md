Status: Active
Source Idea Path: ideas/open/prealloc-stack-layout-contract-boundary.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Stack And Frame Boundaries

# Current Packet

## Just Finished

Lifecycle activation created this execution state from
`ideas/open/prealloc-stack-layout-contract-boundary.md`.

## Suggested Next

Start Step 1 by auditing stack/frame public boundaries and
`stack_layout/coordinator.cpp` responsibilities. Record the boundary map and
the first safe implementation packet in this file before changing code.

## Watchouts

- Keep the first packet audit-oriented unless a small behavior-preserving
  helper or contract cleanup is obvious.
- Do not create forwarding headers or broad include churn.
- Do not change stack layout, frame sizing, slot assignment, alignment,
  dynamic address semantics, or prepared frame dump meaning.

## Proof

No validation has run for this newly activated plan.
