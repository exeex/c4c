# Execution State

Status: Active
Source Idea Path: ideas/open/61_stack_frame_and_addressing_consumption.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Lock The Prepared Addressing Contract
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Activated idea 61 into `plan.md` and reset the execution scratchpad for the
new runbook; no executor packet has advanced Step 1 yet.

## Suggested Next

Execute Step 1 by defining the consumer-oriented prepared frame/addressing
contract in `src/backend/prealloc/prealloc.hpp`, including lookup helpers that
x86 can query without rebuilding local-slot offsets or memory provenance.

## Watchouts

- Keep activation scope on idea 61 only. Do not reopen closed idea 58 or
  activate blocked idea 59 through the runbook text.
- Treat value-home and move-bundle ownership as idea 60 scope, not something
  to absorb into this plan.
- Prefer prepared frame/address data over x86-local slot-name, suffix, or
  offset reconstruction.

## Proof

Not run yet for idea 61 activation.
