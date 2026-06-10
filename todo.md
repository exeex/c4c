Status: Active
Source Idea Path: ideas/open/167_route1_producer_constant_oracle_thinning.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Candidate Route 1 Consumers

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from Step 1 of
`ideas/open/167_route1_producer_constant_oracle_thinning.md`.

## Suggested Next

Execute Step 1: inventory Route 1 producer, constant, and materialization
consumer candidates; select one bounded consumer family; record target files,
proof command, and negative cases here before implementation begins.

## Watchouts

- Keep prepared producer and constant helpers visible as migration oracles until
  the selected consumer no longer needs them.
- Do not move homes, registers, storage, emitted-register state, operand views,
  frame slots, final instruction order, spill/reload behavior, or publication
  hooks into BIR.
- Reject testcase-shaped shortcuts, expectation rewrites, or narrow named-case
  matching as progress.

## Proof

Not run. This activation is lifecycle-only.
