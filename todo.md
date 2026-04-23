Status: Active
Source Idea Path: ideas/open/88_prepared_frame_stack_call_authority_completion_for_target_backends.md
Source Plan Path: plan.md
Current Step ID: 3.2
Current Step Title: Save, Clobber, And Preservation Authority
Plan Review Counter: 6 / 6
# Current Packet

## Just Finished

Completed another bounded Step 3.2 "Save, Clobber, And Preservation
Authority" packet for idea 88 by auditing the remaining scalar prepared
save/clobber/preservation surface and tightening proof around the existing
direct call-clobber publication instead of widening into grouped-register
work.

Current packet result:
- audited the owned scalar Step 3.2 surface and did not find another truthful
  prepared field to add without crossing into grouped-register scope
- focused printer coverage now proves the direct prepared call-clobber detail
  line publishes bank/register/width/unit occupancy authority, not just the
  summary token
- the scalar save/clobber/preservation contract remains unchanged in code, and
  this slice makes the current Step 3.2 publication boundary explicit in proof

## Suggested Next

Ask the supervisor whether Step 3.2 should now hand off for reviewer scrutiny
or plan-owner routing, because the remaining obvious expansion path is
grouped-register authority from idea 89 rather than another bounded scalar
prepared-surface gap.

## Watchouts

- Keep scalar frame/stack/call authority separate from grouped-register work in
  idea 89.
- Keep call-boundary authority at the prepared contract boundary; do not turn
  this packet into target-specific call instruction recovery.
- Keep Step 3.2 focused on scalar preservation and clobber facts already known
  to prepared frame/regalloc state; do not widen into grouped-register routes.
- Treat proof hardening as confirmation of the current scalar boundary, not as
  permission to backfill grouped-register banks/spans under Step 3.2.
- If a follow-up packet claims a new scalar gap remains, make it name the
  missing direct prepared fact concretely before editing contract fields.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer)$'`.
Result: pass.
Proof log: `test_after.log`.
