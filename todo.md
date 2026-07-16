Status: Active
Source Idea Path: ideas/open/855_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select The Next Body-Parameter Authority Row

# Current Packet

## Just Finished

Completed `plan.md` Step 1 by selecting exactly one next currently produced
function-body parameter-use row after accepted 734 Step 7.43:
`LirBinOp.scalar_lhs_parameter_authority` for a current-function
`DirectScalar` floating parameter used as the LHS of a binary floating
multiply (`fmul`). The selected authority tuple is the original parameter
`LirValueId`, current `LirFunction.link_name_id` owner, parameter index,
matching floating `LirTypeRef`, `LirNativeBodyParameterAbi::DirectScalar`,
and explicit `LirScalarBinaryParameterRole::Lhs`. The selected consumer
relation is a `LirBinOp` whose opcode is `fmul`, whose `lhs` is the same
parameter SSA/value, whose `type_str` matches the authority type, and whose
`rhs` is a nonselected scalar operand.

## Suggested Next

Execute `plan.md` Step 2 for only the selected binary-`fmul` LHS row: publish
or tighten the native producer/schema/verifier contract as needed and add
focused positive plus malformed-authority coverage for the selected tuple and
consumer relation. Do not implement the Raw-BIR receiver.

## Watchouts

Accepted rows through 734 Step 7.43 remain excluded: DirectPointer GEP,
DirectScalar binary LHS/RHS, DirectScalar return value, DirectScalar switch
selector, DirectScalar truthiness-comparison LHS, fixed direct-call arguments
0 and 1, DirectPointer pointer truthiness, and DirectScalar unary `fneg`.
The selected binary-`fmul` LHS row reuses the native scalar-LHS authority shape
but is a distinct consumer relation. Later or nonselected candidates remain
fail closed: binary RHS for this row, other binary opcodes, comparison
variants, additional call arguments beyond the established argument-0/1
contracts, aggregate/vector, memory/VA, module/type/global/metadata, residual
instruction/terminator, inline assembly, byval/non-direct ABI forms, and any
row that would require text, names, rendered operands, signatures,
diagnostics, compatibility mirrors, `monostate`, or testcase shape.

## Proof

`git diff --check`
