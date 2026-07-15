# LIR Anonymous Aggregate Layout Type Facts

Status: Open
Type: bounded structured type-model prerequisite
Blocks: `ideas/open/754_lir_aggregate_vector_value_identity_convergence.md` Step 3

## Goal

Publish checked native field-layout and field-type facts for anonymous LIR
aggregate types, so a bounded downstream row can validate aggregate field
indices and result element types without interpreting `LirTypeRef` display
text.

## Why This Exists

754's selected direct-complex `LirExtractValueOp` has
`agg_type = LirTypeRef("{ float, float }")`. Existing structured type support
covers arrays and named structs but does not retain anonymous composite field
lists. Therefore index bounds and selected-element type cannot be verified
without forbidden compatibility-text parsing.

## In Scope

- Trace the smallest type construction, ownership, and verifier seams for
  anonymous aggregate field lists used by the selected direct-complex path.
- Select and implement the minimum native anonymous aggregate layout/type fact
  representation, with checked construction and compatibility rendering.
- Prove native field-count/field-type access and rejection of malformed or
  incoherent anonymous layout facts, then publish the exact 754 handoff.

## Out Of Scope

- `LirExtractValueOp` result, aggregate-use, field-index, or result-type
  validation; 754 owns that row work after this handoff.
- Raw-BIR, other aggregate/vector row conversion, generic type-system rewrite,
  target lowering, MIR, emission, or identity recovery from display text.

## Acceptance Criteria

- Anonymous aggregate layouts retain checked native ordered field-type facts
  sufficient for a consumer to establish bounds and selected field type.
- Missing, malformed, foreign, or type-incoherent layout facts reject without
  parsing compatibility strings.
- Focused positive and malformed proof publishes a precise 754-consumable
  handoff; existing named-struct/array paths remain unchanged or fail closed.

## Reviewer Reject Signals

- Reject parsing `{ ... }` text, printer output, LLVM text, instruction order,
  or testcase names to obtain fields or types.
- Reject adding `LirExtractValueOp` row validation, a Raw-BIR receiver, or a
  broad aggregate/vector conversion under this prerequisite.
- Reject an abstraction-only carrier that leaves the direct-complex anonymous
  aggregate path without checked native field facts.
- Reject testcase-shaped shortcuts, expectation downgrades, or weaker verifier
  contracts claimed as native layout/type progress.
