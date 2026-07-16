Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.43
Current Step Title: Receive DirectScalar Fneg Parameter Authority

# Current Packet

## Just Finished

Closed 854 and reactivated 734 after the accepted DirectScalar unary-`fneg`
producer/schema/verifier handoff. Steps 1 through 7.42 remain accepted
historical receiver work and must not be repeated.

## Suggested Next

Implement only Step 7.43: receive the closed-854 tuple for an original
current-function DirectScalar parameter used as unary floating `fneg` `lhs`,
including parameter `LirValueId`, owner, parameter index, matching
`LirTypeRef`, `DirectScalar` ABI, explicit `Lhs` role, unary `LirBinOp`
opcode `fneg`, parameter SSA/value as `lhs`, and empty `rhs`.

## Watchouts

Do not edit LIR producer/schema code, repeat accepted parameter rows through
pointer truthiness, receive another parameter form, or recover authority from
operand text, opcode spelling, signatures, names, diagnostics, compatibility
mirrors, rendered output, or testcase shape.

## Proof

No Step 7.43 proof has run yet. Required after implementation: fresh build,
focused backend receiver proof, matching regression guard, and
`git diff --check`.
