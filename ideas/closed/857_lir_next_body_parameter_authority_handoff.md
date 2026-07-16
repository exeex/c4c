# LIR Next Body-Parameter Authority Handoff

Status: Closed
Type: bounded LIR producer/schema/verifier authority publication
Predecessor: `ideas/open/734_lir_to_new_bir_container_completeness.md`
Consumer: `ideas/open/734_lir_to_new_bir_container_completeness.md`

## Goal

Publish one exact structured LIR authority handoff for the next valid
function-body parameter-use row after 734's accepted Step 7.45 DirectScalar
binary-`fmul` RHS receipt.

## Why This Exists

Idea 734 cannot continue receiving body-parameter rows unless LIR first
publishes native identity, owner, type, ABI, role, and consumer coherence for a
specific current-function parameter use. Accepted Step 7.45 completed only the
closed-856 RHS `fmul` row; the next row must be selected and proved at the
producer/schema/verifier layer before Raw-BIR receipt is authorized.

## In Scope

- Trace the next valid function-body parameter-use row after the accepted
  DirectPointer and DirectScalar body-parameter receipts through Step 7.45.
- Publish only the native structured authority required for that one selected
  row, including parameter source identity, current-function ownership,
  parameter index, exact type, native body-parameter ABI, role, and consumer
  relation coherence.
- Add focused verifier coverage for present, missing/omitted, invalid,
  duplicate, foreign-owner, wrong-index, wrong-type, wrong-ABI, wrong-role, and
  selected-consumer incoherence forms as applicable to the selected row.
- Record an exact one-row handoff back to 734 only after focused proof accepts
  the producer/schema/verifier contract.

## Out Of Scope

- Raw-BIR containers, builders, importers, verifier receipt, or receiver tests.
- Reopening accepted DirectPointer or DirectScalar body-parameter receipts
  through 734 Step 7.45.
- Selecting from display text, names, rendered operands, signatures,
  compatibility mirrors, `monostate`, or testcase shape.
- Memory/VA, aggregate/vector, module/type/global/metadata, residual
  instruction/terminator, inline-assembly, or more than one body-parameter row.
- Weakening unsupported diagnostics, expectation contracts, or fail-closed
  malformed-authority behavior.

## Acceptance Criteria

- Exactly one next valid function-body parameter-use row is selected and named
  with its native producer surface and consumer relation.
- LIR publishes all structured authority needed for that row without Raw-BIR
  receiver changes.
- The LIR verifier rejects malformed, ambiguous, foreign, role/type/ABI
  incoherent, and selected-consumer-incoherent authority before downstream use.
- Focused same-feature positive and malformed-authority tests pass with
  supervisor-accepted proof and `git diff --check`.
- The handoff back to 734 states the exact tuple, proof, and receiver
  exclusions; all nonselected rows remain fail closed.

## Reviewer Reject Signals

- Reject Raw-BIR/importer edits or claiming a receiver receipt under this
  producer handoff idea.
- Reject repeating or relabeling accepted DirectPointer or DirectScalar
  body-parameter rows through Step 7.45 as new progress.
- Reject deriving authority from text, names, rendered operands, signatures,
  compatibility mirrors, `monostate`, or testcase-shaped selectors.
- Reject publishing a generic body-parameter authority bucket instead of one
  selected row with explicit owner, index, type, ABI, role, and consumer
  coherence.
- Reject weakening verifier diagnostics, unsupported contracts, malformed
  rejection, or fail-closed behavior for nonselected rows.
- Reject absorbing memory/VA, aggregate/vector, module/type/global/metadata,
  residual instruction/terminator, inline-assembly, or any second row.

## Closure Record

Disposition: capability complete for this bounded producer/schema/verifier
handoff. Implementation commit `6c11a15c1` proves exactly one selected row:
`LirBinOp.scalar_lhs_parameter_authority` for a current-function
`DirectScalar` floating parameter used as the LHS of binary `fadd`, with
producer shape equivalent to `return x + 2.0;`.

The handed-off native tuple is the original parameter `LirValueId`, current
`LirFunction.link_name_id` owner, parameter index, matching floating
`LirTypeRef`, `LirNativeBodyParameterAbi::DirectScalar`, and explicit
`LirScalarBinaryParameterRole::Lhs`. The consumer relation is `LirBinOp`
opcode `fadd`; `lhs` is the same parameter SSA/value as the authority tuple;
`type_str` matches the authority type; and `rhs` is a nonselected scalar
operand.

The verifier admits only the selected floating `fadd` LHS authority, requires
a nonselected scalar RHS, and rejects duplicate selected floating-`fadd` LHS
consumers in the current function. Malformed coverage rejects
omitted/missing, invalid, duplicate definition, foreign owner, wrong index,
wrong type, wrong ABI, wrong role, non-`fadd`, LHS mismatch, type mismatch,
selected-RHS incoherence, and duplicate selected consumer forms. Neighboring
`fmul` negative coverage uses nonselected `fsub` instead of now-selected
`fadd`.

Accepted proof:

```
( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_function_signature_type_ref$' ) > test_after.log 2>&1 && git diff --check
```

The focused regression guard passed 1/1. The broader shared-verifier matching
before/after guard

```
( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_' )
```

passed 7/7 before and after.

Return to `ideas/open/734_lir_to_new_bir_container_completeness.md` for a
future bounded Raw-BIR receiver packet that receives only this selected
binary-`fadd` LHS DirectScalar parameter-use row into typed Raw BIR. Raw-BIR
receiver implementation did not start here.
