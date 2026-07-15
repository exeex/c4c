# Current Packet

Status: Active
Source Idea Path: ideas/open/795_lir_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace the selected parameter-index body-use handoff
你該做code review了

## Just Finished

810 Steps 1--2 are accepted (`f1cb9c510`, `1f1a1fb38`), but its required
post-commit full baseline rejected at 3035/3037. The variable parameter-index
GEP failure is switched here; the independent PHI failure remains owned by
806.

## Suggested Next

Trace the `pr21173.c` variable RHS from authoritative GEP-index verification
to its native function-body parameter publication and lowering handoff. Keep
the selected route to one body-parameter/ABI classification surface.

## Watchouts

810's native-immediate RHS repair is accepted and must not be reopened. Do not
use rendered parameter identity, weaken GEP verification, or absorb the
`20060910-1.c` PHI producer failure owned by 806. A focused result does not
clear the rejected 3035/3037 baseline.

## Proof

No 795 proof recorded yet. The outgoing 810 focused
`frontend_lir_call_type_ref` guard was accepted at 0/1 before to 1/1 after;
the post-commit full baseline remains rejected at 3035/3037.
