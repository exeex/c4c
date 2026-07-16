Status: Active
Source Idea Path: ideas/open/856_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace And Select One Next Body-Parameter Use Authority Row
你該做code review了

# Current Packet

## Just Finished

Completed `plan.md` Step 1 by tracing the remaining current LIR
body-parameter authority matrix after accepted 734 Step 7.44 receiver commit
`e0540da75`.

Selected exactly one next currently produced receiver-ready row:
`LirBinOp.scalar_rhs_parameter_authority` for a current-function
`DirectScalar` floating parameter used as the RHS of binary floating multiply
`fmul`, e.g. a producer shape equivalent to `return 2.0 * x;`. The native
tuple is already representable without presentation recovery: original
parameter `LirValueId`, current `LirFunction.link_name_id` owner, parameter
index, matching floating `LirTypeRef`, `LirNativeBodyParameterAbi::DirectScalar`,
and explicit `LirScalarBinaryParameterRole::Rhs`.

Selected consumer relation: `LirBinOp` opcode `fmul`; `rhs` is the same
parameter SSA/value as the authority tuple; `type_str` matches the authority
type; and `lhs` is a nonselected scalar operand. This is distinct from the
accepted Step 7.44 `LirBinOp.scalar_lhs_parameter_authority` fmul-LHS row.

## Suggested Next

Execute `plan.md` Step 2 for only the selected binary-`fmul` RHS row. Add or
tighten producer/verifier coverage proving the selected
`LirBinOp.scalar_rhs_parameter_authority` DirectScalar floating RHS tuple and
its fmul consumer coherence, with focused positive and malformed coverage for
omitted/missing, invalid, duplicate, foreign, owner/index/type/ABI/role, non-
`fmul`, RHS mismatch, result/type mismatch, LHS selected-parameter incoherence,
and duplicate-consumer forms as applicable. Do not start Raw-BIR receiver work.

## Watchouts

Exclude accepted rows through 734 Step 7.44: DirectPointer GEP-base,
DirectScalar binary-LHS/RHS integer row, return-value, switch-selector,
truthiness-comparison LHS, fixed-direct-call arguments 0 and 1,
DirectPointer pointer-truthiness, DirectScalar unary-`fneg` LHS, and
DirectScalar binary-`fmul` LHS. The selected row may reuse the existing RHS
carrier schema, but the handoff must be only the distinct binary-`fmul` RHS
consumer row.

Later or nonselected candidates remain fail closed: memory/VA, aggregate/vector,
module/type/global/metadata, residual instruction/terminator, inline-assembly,
ABI-expanded or aggregate parameters, direct-call argument positions beyond the
bounded accepted slots, and generic parameter sweeps have no selected
one-row consumer handoff here. Do not derive authority from text, names,
rendered operands, signatures, diagnostics, compatibility mirrors, `monostate`,
or testcase shape.

## Proof

Analysis and todo-only selection. Required check: `git diff --check`.
