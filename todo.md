Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.48
Current Step Title: Receive the one 859-authorized DirectScalar binary-fsub-LHS parameter authority row
你該做code review了
你該做test baseline review了

# Current Packet

## Just Finished

Closed 859 after accepted producer handoff commit `c3a7883a4` and reactivated
734 for the matching Raw-BIR receiver packet.

## Suggested Next

Receive only the closed-859 `LirBinOp.scalar_lhs_parameter_authority` floating
`fsub` LHS DirectScalar parameter-use row into typed Raw BIR, preserving the
native parameter tuple and binary consumer coherence.

## Watchouts

Do not repeat Step 7.47, edit LIR producer authority, receive floating
binary-`fsub` RHS, other floating binary parameter uses, memory/VA,
aggregate/vector, module/type/global/metadata, residual
instruction/terminator, inline-assembly, ABI-expanded or aggregate
parameters, generic parameter sweeps, or any other family.

## Proof

No proof has run for Step 7.48 yet.
