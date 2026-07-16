# LIR Direct HIR Family Construction and Array Composition

Status: Open
Type: first-owner producer construction and recursive array composition
Matrix Rows: M3, M11
Dependencies: 838, 839, 840, 841, 842 at explicit value boundaries

## Goal

Replace semantic construction from `llvm_ty()` and rendering with producer-
specific construction from `TypeSpec`, canonical owners/layout, vector facts,
and signature facts; retain arrays as recursive typed composition.

## In Scope

- Migrate bounded HIR-to-LIR producer groups, `emit_rval_*`, and `coerce`.
- Make array length and typed element refs recursive facts; prove GEP consumes
  typed elements.

## Out Of Scope

- Parsing printer output, migrating global/extern mirrors, collector scans, or
  deleting compatibility adapters whose named producer is not migrated.

## Acceptance Criteria

- Fresh build plus scalar/vector/array/nested aggregate/signature lowering,
  GEP verification, and proof no rendered output is reparsed.
- Delete each runtime-text/rendered aggregate/array factory only after its
  exact producer emits a family ref; retain parser/extern/asm adapters by
  named consumer.

## Reviewer Reject Signals

- Reject `llvm_ty()`/printer text as authority, flattened recursive children,
  broad producer rewrites, or factory deletion before named-consumer parity.
