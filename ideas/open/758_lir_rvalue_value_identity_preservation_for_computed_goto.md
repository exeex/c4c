# LIR Rvalue Value Identity Preservation For Computed-Goto

Status: Open (active prerequisite for idea 757)
Type: bounded LIR rvalue/operand typed-identity preservation repair
Predecessor: `ideas/open/757_lir_computed_goto_address_value_identity_publication.md`

## Goal

Preserve existing current-function typed `LirValueId` identity for local and
parameter rvalues across the established rvalue-expression and rvalue-operand
route, so the computed-goto producer can consume that identity without
recovering it from operand spelling.

## Why This Exists

The active computed-goto producer obtains an operand through
`emit_rval_operand`, but that route currently reduces a local `DeclRef` to
`LirOperand::raw(emit_rval_expr(...))`. Although
`emit_rval_payload(DeclRef)` emits the typed `%dispatch` `LirLoadOp`, it
returns only display spelling. Idea 757 therefore cannot obtain an address
`LirValueId` without violating its no-text-recovery rule.

## In Scope

- preserve the already-created typed value identity for local and parameter
  rvalues through the existing `emit_rval_payload` / `emit_rval_expr` /
  `emit_rval_operand` route
- retain textual operand spelling only as display compatibility, never as a
  semantic substitute
- make the preserved result available to immediate route consumers without
  adding computed-goto-specific LIR schema or verifier authority
- add nearby same-route coverage proving local and parameter identity remains
  available and cannot be restored from misleading display spelling
- record an exact typed-identity handoff for 757

## Out Of Scope

- adding or verifying `LirIndirectBrOp` address fields, or publishing the 757
  computed-goto producer handoff
- Raw-BIR containers, importer, verifier, receiver, target lowering, MIR, or
  emission
- new value IDs, changes to load semantics, object/lifetime authority, PHI,
  CFG successor authority, aggregate/vector, memory/va, or unrelated rvalue
  families
- parsing operands, labels, printer output, rendered LLVM, or testcase names
  to recreate identity

## Acceptance Criteria

- A local or parameter rvalue that already produces a typed current-function
  value retains that exact identity through the current rvalue/operand route.
- Consumers can distinguish absent, invalid, foreign, and unsuitable identity
  from display spelling and fail closed without text fallback.
- Focused same-route positive and malformed-identity tests include misleading
  display text and establish that it cannot select or repair semantic identity.
- The handoff identifies the preserved typed result, its ownership/failure
  boundary, and the exact action that returns control to 757 Step 1.

## Reviewer Reject Signals

- Reject operand-string, rendered-output, label, printer, or testcase-name
  parsing used to derive a `LirValueId` or type.
- Reject a computed-goto address field/verifier, Raw-BIR receiver work, or
  changes to successor authority claimed as progress for this prerequisite.
- Reject a named-case-only fix, expectation downgrade, helper rename, or
  display agreement claimed as typed identity preservation.
- Reject a route that still drops the emitted local/parameter value identity
  and merely renames the resulting raw spelling, or that broadens into new
  value creation or unrelated rvalue families.
