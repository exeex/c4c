Status: Active
Source Idea Path: ideas/open/88_prepared_frame_stack_call_authority_completion_for_target_backends.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Contract Surface Audit
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Plan activated for idea 88. No executor packet has landed yet.

## Suggested Next

Start Step 1 by auditing the current prepared `frame_plan`, `dynamic_stack_plan`,
`call_plans`, and target-backend consumers to isolate the first scalar contract
gap that still forces backend-local reconstruction.

## Watchouts

- Keep scalar frame/stack/call authority separate from grouped-register work in
  idea 89.
- Do not expand this runbook into target-specific instruction spelling or
  generic x86 behavior recovery.
- If the first bad fact is still upstream of BIR/prepared publication, record
  that explicitly before changing target consumers.

## Proof

Not run yet. Activation only.
