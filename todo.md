Status: Active
Source Idea Path: ideas/open/372_rv64_object_route_frame_slot_address_call_args.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Frame-Slot Address Call Arguments

# Current Packet

## Just Finished

Activated `ideas/open/372_rv64_object_route_frame_slot_address_call_args.md`
as the active runbook.

## Suggested Next

Execute Plan Step 1 by auditing prepared call-plan and frame-slot address
materialization facts for:

- `src/20000217-1.c`
- `src/va-arg-13.c`

Record which observed failures are in-scope frame-slot address call arguments
and which are blocked earlier by `ideas/open/374_rv64_object_route_non_register_param_homes.md`.

## Watchouts

- Do not implement pointer-value local-memory or frame-slot payload-value call
  argument support in this route; ideas 368 and 373 already own those.
- Do not absorb non-register parameter homes, aggregate `va_arg`, byval homes,
  terminators, or source-shape shortcuts.
- The address path must materialize the stack-slot address selected by the
  prepared call plan, not load the payload stored in that slot.
- Put audit logs under `build/agent_state/`; root `test_after.log` is reserved
  for delegated proof.

## Proof

Lifecycle-only activation. No build proof required.
