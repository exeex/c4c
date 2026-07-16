Status: Active
Source Idea Path: ideas/open/854_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select The Next Body-Parameter Authority Row

# Current Packet

## Just Finished

Step 1 selected exactly one next function-body parameter-use authority row
after 734's accepted DirectPointer pointer-truthiness receipt: a native
current-function `DirectScalar` parameter used as the operand of unary
floating `fneg`, carried on `LirBinOp.scalar_lhs_parameter_authority` with
explicit `Lhs` role and consumer coherence to a unary `LirBinOp` whose opcode
is `fneg`, whose `lhs` is the parameter SSA/value, whose type matches the
parameter `LirTypeRef`, and whose `rhs` is empty.

This selection does not reopen accepted rows. DirectPointer GEP,
DirectScalar binary-LHS, binary-RHS, ReturnValue, switch-selector,
truthiness-comparison-LHS, fixed-direct-call argument-0, fixed-direct-call
argument-1, and DirectPointer pointer-truthiness already have accepted
producer/receiver history and remain historical evidence only. The selected
unary `fneg` row is distinct from the accepted binary-LHS receiver because its
consumer relation is unary floating negation, not a two-operand scalar binary
operation.

## Suggested Next

Execute Step 2 for only the selected unary floating `fneg` row: publish or
confirm the native authority contract for the original parameter `LirValueId`,
current-function owner, parameter index, matching `LirTypeRef`,
`DirectScalar` ABI, explicit `Lhs` role, and unary-`fneg` consumer coherence;
then add/refresh focused positive and malformed-authority proof as needed.

## Watchouts

Do not edit Raw-BIR/importer code in 854. Do not reopen accepted
body-parameter rows or infer authority from text, names, signatures,
diagnostics, rendered operands, compatibility mirrors, or testcase shape.
Nonselected parameter forms remain fail closed: ordinary scalar comparisons
other than the accepted truthiness LHS, DirectPointer uses other than accepted
GEP and pointer-truthiness, additional fixed-call arguments, generic
`LirBinOp.lhs` two-operand binary reuse, declaration-only/signature-only
parameters, memory/VA, aggregate/vector, and residual instruction/terminator
uses are not selected by Step 1.

## Proof

Step 1 was inspection and `todo.md` selection only. Required proof:
`git diff --check`.
