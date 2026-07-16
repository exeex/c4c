# LIR Nominal Function Signature and Call Composition

Status: Open
Type: first-owner function signature schema and call/declaration migration
Matrix Rows: M8, M9
Dependencies: 838 aggregate refs; preserves 761; coordinates 795 and 829/830 without owning body-use identity

## Goal

Replace partial signature/text mirrors with a nominal function-signature store
that composes legal family refs for declarations and calls.

## In Scope

- Add `LirFunctionSignatureRef`/store for return, ordered parameters, variadic,
  and ABI facts; migrate declaration and call composition.
- Use only the later restricted value carrier at actual argument/result
  boundaries; preserve raw extern/inline-asm compatibility as named one-way
  adapters.

## Out Of Scope

- 829/830 native body-use/argument identity, unrestricted value unions, and
  scalar/vector/aggregate producer migration.

## Acceptance Criteria

- Fresh build and focused declaration/call lowering, fixed/variadic
  verification, aggregate parameter/return, raw-call compatibility, printer,
  and reference-collector proof; reject malformed signatures and wrong-module
  aggregate alternatives.
- Delete semantic `signature_text`, `args_str`, parsed-call construction, and
  duplicate `arg_type_refs` only after named declarations, calls, verifier,
  printer, and collectors consume signature/value facts.

## Reviewer Reject Signals

- Reject parser/text/signature spelling as semantic construction or a 829/830
  authority claim.
- Reject generic call rewrites, downgraded contracts, or raw compatibility that
  remains a semantic fallback after its consumer migrates.
