Status: Active
Source Idea Path: ideas/open/07_aarch64_call_boundary_move_emission_only.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Remaining Selection Decisions In Call Moves

# Current Packet

## Just Finished

Lifecycle activation created the runbook for Step 1. No executor packet has
completed yet.

## Suggested Next

Start Step 1 by mapping remaining source-selection decisions in
`calls_moves.cpp` and related calls helpers.

## Watchouts

- Keep source-selection authority in prepared call facts, not AArch64 emission.
- Do not touch the transient `review/` artifacts unless explicitly delegated.
- Treat expectation weakening or named-test shortcuts as route failures.

## Proof

Lifecycle-only activation. No build proof required.
