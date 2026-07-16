# LIR Next Non-Body-Parameter Authority Handoff

Status: Open
Type: producer/schema/verifier handoff for one next non-body-parameter LIR row
Parent Source: ideas/open/734_lir_to_new_bir_container_completeness.md
Supersedes No-Change Route: ideas/closed/862_lir_next_body_parameter_authority_handoff.md

## Goal

Trace, publish, verify, and hand off exactly one next valid current-LIR
semantic row outside the exhausted function-body parameter authority surface
after accepted 734 Step 7.49.

## Why This Exists

Idea 734 remains incomplete after accepted Step 7.49, but the attempted 862
body-parameter successor found no unaccepted current-LIR body-parameter row
inside its source scope. DirectPointer GEP/truthiness and DirectScalar unary,
return, switch, comparison/truthiness, direct-call, and binary `fmul`/`fadd`/
`fsub` rows are already accepted or explicitly excluded from reopening.

The next executable route must therefore select a separate non-body-parameter
family before 734 can receive another Raw-BIR row.

## In Scope

- Inspect current LIR producer and verifier behavior for one non-body-
  parameter family still missing a typed 734 disposition.
- Select exactly one valid current-LIR semantic row with native structured
  authority that can be published and verified without presentation recovery.
- Add only the LIR producer/schema/verifier fields and checks required for the
  selected row.
- Verify the row's native identity, current-function or module ownership as
  applicable, typed `LirTypeRef` or other structured type facts, explicit role,
  and selected consumer relation.
- Add focused malformed-authority coverage for absent, invalid, duplicate,
  foreign, owner/type/role-incoherent, and consumer-incoherent forms relevant
  to the selected row.
- Hand off the exact row, fields, proof, and malformed matrix back to 734 for
  one later Raw-BIR receiver packet.

## Out Of Scope

- Raw-BIR containers, builders, views, importer dispatch, verifier, or backend
  receiver tests.
- Any function-body parameter-use row, ABI-expanded/aggregate parameter row,
  or accepted DirectPointer/DirectScalar body-parameter route.
- Generic residual sweeps, memory/VA sweeps, aggregate/vector sweeps,
  module/type/global/metadata sweeps, CFG/PHI rewrites, inline-assembly
  template/constraint parsing, target-lowering work, or final 797 convergence.
- Recovering authority from text, names, rendered operands, signatures,
  diagnostics, compatibility mirrors, `monostate`, or testcase shape.
- Reworking older open ideas such as 795 or 796 unless the supervisor
  explicitly chooses one as the selected row's owner and reconciles its stale
  return records first.

## Acceptance Criteria

- Exactly one next valid non-body-parameter row is selected and documented.
- The selected row has native structured LIR authority for its identity/type
  tuple and selected consumer relation.
- The LIR verifier rejects malformed authority before printing or downstream
  use.
- Focused producer/verifier coverage proves the positive row and nearby
  malformed matrix.
- The handoff names the exact 734 return action and states which families
  remain unsupported or separately scoped.
- Supervisor-selected focused proof and any required matching regression guard
  are accepted.

## Reviewer Reject Signals

- Reject any Raw-BIR/importer/container/verifier receiver edit; that work
  belongs to 734 after this handoff closes.
- Reject selecting a function-body parameter row, multiple rows, a generic
  residual sweep, or any row already accepted by 734 through Step 7.49.
- Reject recovering identity, role, type, opcode, operand relation, or
  consumer coherence from rendered text, names, signatures, diagnostics,
  compatibility mirrors, `monostate`, or testcase-specific shape.
- Reject expectation downgrades, unsupported-to-supported label changes,
  helper renames, or classification-only edits claimed as authority
  publication.
- Reject broad LIR schema churn, target-lowering behavior, final convergence,
  or unrelated memory/VA, aggregate/vector, module/type/global/metadata, CFG/
  PHI, instruction/terminator, or inline-assembly work.
