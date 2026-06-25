Status: Active
Source Idea Path: ideas/open/373_rv64_object_route_frame_slot_value_call_args.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Frame-Slot Value Call Arguments

# Current Packet

## Just Finished

Activated `ideas/open/373_rv64_object_route_frame_slot_value_call_args.md` into
`plan.md` and reset canonical execution state for Step 1.

## Suggested Next

Execute Step 1 by auditing prepared call-plan, value-home, and frame-slot facts
for `src/20000121-1.c`. Store any probe artifacts under `build/agent_state/`
and record the first supportable scalar frame-slot-value call-argument shape
for implementation.

## Watchouts

- Do not include frame-slot address call arguments; those belong to idea 372.
- Do not reopen pointer-value local-memory support from closed idea 368.
- Do not handle aggregate `va_arg`, byval homes, terminators, or source-shape
  shortcuts in this route.
- Progress must be based on prepared value-home and call-plan metadata.

## Proof

Lifecycle activation only; no build proof required.
