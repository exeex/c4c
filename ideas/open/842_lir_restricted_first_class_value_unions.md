# LIR Restricted First-Class Value Boundary Unions

Status: Open
Type: first-owner bounded polymorphic value carrier
Matrix Rows: M10
Dependencies: 838, 840, 841; preserves 775/776 and 754

## Goal

Define exact boundary-local tagged value unions for call argument/result, PHI,
select, and return rather than retaining a universal type carrier.

## In Scope

- Add explicit enum kinds, C++20 traits, and checked access for only admitted
  scalar/vector/aggregate and separately evidenced pointer alternatives.
- Migrate named boundaries with exhaustive verifier, printer, coercion, and
  BIR receipt handling.

## Out Of Scope

- A universal variant/ID, implicit cross-family adapters, or signature/body-use
  ownership outside the named boundaries.

## Acceptance Criteria

- Fresh build and exhaustive valid/malformed/wrong-kind proof for every named
  boundary, including coercion and receiver coverage.
- Delete boundary `LirTypeRef` fields only after all named boundaries use
  exhaustive checked alternatives.

## Reviewer Reject Signals

- Reject unbounded alternatives, RTTI/vtables, missing enum cases, generic
  IDs, testcase-only handling, or retaining the universal bag behind a union.
