Status: Active
Source Idea Path: ideas/open/prealloc-call-boundary-ordering-republication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Adapt AArch64 Call Consumption

# Current Packet

## Just Finished

Completed Step 3: `Adapt AArch64 Call Consumption`.

Recorded the Step 3 boundary after the before-call and after-call explicit move
adapters. Both explicit call-boundary move paths now construct and read
`plan_prepared_call_boundary_effects` through narrow AArch64 adapters while
preserving the existing target-local lowering for register views, memory
operands, scratch selection, byval/result publication handling, and final
instruction record construction.

Preservation-home population and republication loops remain target-local for
this plan. Their current behavior is coupled to existing ordering and
register-emission policy, including preservation-home population before
explicit before-call moves and target-side republication after calls, so further
AArch64 call rewiring is intentionally deferred.

## Suggested Next

Proceed to Step 4: cross-target ordering proof for the neutral
call-boundary-effect plan, without claiming additional AArch64 call lowering
migration.

## Watchouts

Keep preservation ordering and republication emission out of Step 3 follow-up
work unless a later source idea defines a tighter neutral ordering/register
identity boundary. The after-call adapter still filters explicit effects and
remaps their indexes because the neutral full effect plan includes
preservation-population records before after-call explicit effects.

## Proof

Audit-only `todo.md` update. No build or test proof was required, and no new
`test_after.log` was created for this non-code packet.
