Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.49
Current Step Title: Receive the one 860-authorized DirectScalar binary-fsub-RHS parameter authority row

# Current Packet

## Just Finished

Reactivated 734 after closed 860 completed the producer-side DirectScalar
floating binary-`fsub` RHS parameter-authority handoff. Accepted 734 Steps 1
through 7.48 remain historical progress and must not be repeated.

## Suggested Next

Execute `plan.md` Step 7.49 only: receive the closed-860
`LirBinOp.scalar_rhs_parameter_authority` floating `fsub` RHS DirectScalar
parameter-use row into typed Raw BIR, preserving the handed-off parameter tuple
and binary consumer coherence.

## Watchouts

Do not edit LIR producer authority, repeat Step 7.48, receive another
parameter-use row, recover authority from presentation text, or absorb
memory/VA, aggregate/vector, module/type/global/metadata, residual
instruction/terminator, inline-assembly, ABI-expanded or aggregate parameters,
generic parameter sweeps, or any other family.

## Proof

No proof has run for Step 7.49 yet. Expected proof starts with a fresh build,
focused backend LIR-to-BIR receiver test, `git diff --check`, and broader
backend proof if shared importer or verifier code is touched.
