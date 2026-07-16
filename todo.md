Status: Active
Source Idea Path: ideas/open/857_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish and verify the selected authority tuple
你該做code review了
你該做test baseline review了

# Current Packet

## Just Finished

Completed `plan.md` Step 1 by tracing the remaining current LIR
body-parameter authority matrix after accepted 734 Step 7.45 receiver commit
`2b7e897ce`.

Selected exactly one next currently valid producer row:
`LirBinOp.scalar_lhs_parameter_authority` for a current-function
`DirectScalar` floating parameter used as the LHS of binary floating add
`fadd`, e.g. a producer shape equivalent to `return x + 2.0;`.

The native tuple is already representable without presentation recovery:
original parameter `LirValueId`, current `LirFunction.link_name_id` owner,
parameter index, matching floating `LirTypeRef`,
`LirNativeBodyParameterAbi::DirectScalar`, and explicit
`LirScalarBinaryParameterRole::Lhs`. The selected consumer relation is
`LirBinOp` opcode `fadd`; `lhs` is the same parameter SSA/value as the
authority tuple; `type_str` matches the authority type; and `rhs` is a
nonselected scalar operand.

The row is distinct from accepted DirectScalar integer add LHS/RHS rows,
unary `fneg`, binary `fmul` LHS, and binary `fmul` RHS. Current producer code
can populate the same LHS carrier, but the LIR verifier still fail-closes
floating LHS authority to selected `fmul` and `fneg` consumers, so Step 2 must
prove the `fadd` LHS row explicitly rather than broadening generic floating
binary authority.

## Suggested Next

Execute `plan.md` Step 2 for only the selected binary-`fadd` LHS row. Add or
tighten producer/verifier coverage proving the selected
`LirBinOp.scalar_lhs_parameter_authority` DirectScalar floating LHS tuple and
its `fadd` consumer coherence, with focused positive and malformed coverage for
omitted/missing, invalid, duplicate, foreign, owner/index/type/ABI/role,
non-`fadd`, LHS mismatch, result/type mismatch, RHS selected-parameter
incoherence, RHS nonselected scalar mismatch/type mismatch, and duplicate
selected-consumer forms as applicable. Do not start Raw-BIR receiver work.

## Watchouts

Exclude accepted rows through 734 Step 7.45: DirectPointer GEP-base,
DirectScalar binary-LHS/RHS integer row, return-value, switch-selector,
truthiness-comparison LHS, fixed-direct-call arguments 0 and 1, DirectPointer
pointer-truthiness, DirectScalar unary-`fneg` LHS, DirectScalar binary-`fmul`
LHS, and DirectScalar binary-`fmul` RHS. The selected row may reuse the
existing LHS carrier schema, but the handoff must be only the distinct
binary-`fadd` LHS consumer row.

Later or nonselected candidates remain fail closed: floating binary-`fadd` RHS,
other floating binary op parameter uses, memory/VA, aggregate/vector,
module/type/global/metadata, residual instruction/terminator, inline-assembly,
ABI-expanded or aggregate parameters, direct-call argument positions beyond
the bounded accepted slots, and generic parameter sweeps have no selected
one-row consumer handoff here. Do not derive authority from text, names,
rendered operands, signatures, diagnostics, compatibility mirrors, `monostate`,
or testcase shape.

## Proof

Analysis and todo-only selection. Required check: `git diff --check`.
