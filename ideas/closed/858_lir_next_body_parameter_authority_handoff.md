# LIR Next Body Parameter Authority Handoff

Status: Closed
Type: producer/schema/verifier handoff for one next function-body parameter-use row
Parent Source: ideas/open/734_lir_to_new_bir_container_completeness.md

## Goal

Trace, publish, verify, and hand off exactly one next valid current-LIR
function-body parameter-use authority row after accepted 734 Step 7.46.

## Why This Exists

Idea 734 can receive only structured LIR authority into typed Raw BIR. Its
latest accepted receiver packet, commit `d5a301ec9`, consumed the closed-857
DirectScalar binary-`fadd` LHS row. The 734 completion gate remains unmet, but
no further receiver row is authorized without a separately accepted
producer-side handoff.

## In Scope

- Inspect current LIR body-parameter producer and verifier behavior after the
  accepted DirectPointer and DirectScalar receipts through 734 Step 7.46.
- Select exactly one next valid function-body parameter-use row that current
  LIR can publish with native structured authority.
- Add only the LIR producer/schema/verifier fields and checks required for
  that one selected row.
- Verify the row's parameter value identity, owner, parameter index, typed
  `LirTypeRef`, native body-parameter ABI, explicit role, and selected
  consumer relation.
- Add focused malformed-authority coverage for absent, invalid, duplicate,
  foreign, owner/index/type/ABI/role-incoherent, and consumer-incoherent
  forms relevant to the selected row.
- Hand off the exact row, fields, proof, and malformed matrix back to 734 for
  one later Raw-BIR receiver packet.

## Out Of Scope

- Raw-BIR containers, builders, views, importer dispatch, verifier, or backend
  receiver tests.
- Reopening any accepted 734 body-parameter receiver row, including
  DirectPointer GEP/truthiness rows and DirectScalar binary, unary, return,
  switch, comparison, direct-call, `fmul`, or `fadd` rows.
- Generic parameter sweeps, ABI-expanded or aggregate parameter families,
  memory/VA, aggregate/vector, module/type/global/metadata, residual
  instruction/terminator, inline-assembly, or target-lowering work.
- Recovering authority from text, names, rendered operands, signatures,
  diagnostics, compatibility mirrors, `monostate`, or testcase shape.

## Acceptance Criteria

- Exactly one next valid body-parameter-use row is selected and documented.
- The selected row has native structured LIR authority for the parameter tuple
  and selected consumer relation.
- The LIR verifier rejects malformed authority before downstream use.
- Focused producer/verifier coverage proves the positive row and nearby
  malformed matrix.
- The handoff names the exact 734 return action and states which rows remain
  unsupported or separately scoped.
- Supervisor-selected focused proof and any required matching regression guard
  are accepted.

## Reviewer Reject Signals

- Reject any Raw-BIR/importer/container/verifier receiver edit; that work
  belongs to 734 after this handoff closes.
- Reject selecting multiple rows, a generic body-parameter family, or any row
  already accepted by 734 through Step 7.46.
- Reject recovering parameter identity, role, type, opcode, operand relation,
  or consumer coherence from rendered text, names, signatures, diagnostics,
  compatibility mirrors, `monostate`, or testcase-specific shape.
- Reject expectation downgrades, unsupported-to-supported label changes,
  helper renames, or classification-only edits claimed as authority
  publication.
- Reject broad LIR schema churn, ABI redesign, target-lowering behavior, or
  unrelated memory/VA, aggregate/vector, module/type/global/metadata,
  instruction/terminator, or inline-assembly work.

## Closure Record

Close accepted. Implementation commit `80d8a339c` proves exactly one selected
producer-side body-parameter row:
`LirBinOp.scalar_rhs_parameter_authority` for a current-function
`DirectScalar` floating parameter used as the RHS of binary floating `fadd`,
with producer shape `return 2.0 + x;`.

The accepted native tuple is the original parameter `LirValueId`, current
`LirFunction.link_name_id` owner, parameter index, matching floating
`LirTypeRef`, `LirNativeBodyParameterAbi::DirectScalar`, and explicit
`LirScalarBinaryParameterRole::Rhs`. The selected consumer relation is
`LirBinOp` opcode `fadd`; `rhs` is the same parameter SSA/value as the
authority tuple; `type_str` matches the authority type; `lhs` is a nonselected
scalar operand.

The existing emitter already publishes the carrier. The verifier now admits
selected floating `fadd` or `fmul` RHS authority, requires a nonselected scalar
LHS, and rejects duplicate selected floating-`fadd` RHS consumers in the
current function. Neighboring `fmul` RHS malformed coverage uses nonselected
`fsub` instead of now-selected `fadd`.

Malformed coverage rejects omitted/missing, invalid, duplicate definition,
foreign owner, wrong index, wrong type, wrong ABI, wrong role, non-`fadd`, RHS
mismatch, type mismatch, selected-LHS incoherence, and duplicate selected
consumer forms.

Accepted proof is the focused
`^frontend_lir_function_signature_type_ref$` producer/verifier proof with
`git diff --check` and regression guard 1/1, plus the broader matching
`^frontend_lir_` before/after guard with 7/7 passing on both sides. No Raw-BIR
receiver work landed in this idea.

Exact 734 return action: reactivate
`ideas/open/734_lir_to_new_bir_container_completeness.md` for a future bounded
Raw-BIR receiver packet that receives only this selected binary-`fadd` RHS
DirectScalar parameter-use row into typed Raw BIR. Do not start Raw-BIR
receiver implementation from this producer handoff.
