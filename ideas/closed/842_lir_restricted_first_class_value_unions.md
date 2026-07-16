# LIR Restricted First-Class Value Boundary Unions

Status: Closed
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

## 866 Reconciliation And 734 Return

Idea 866 keeps this as the ordered producer/schema successor for restricted
first-class boundary alternatives. Return to 734 only after a named boundary
union accepts one exact typed handoff for call, PHI, select, or return receipt;
unbounded value carriers and generic receiver work remain deferred.

## Closure Note

Disposition: intentionally concluded as a bounded PHI boundary capability
slice, not as terminal completion of every first-class value boundary.

Accepted commits:

- `464fa3b92` docs: select restricted PHI boundary target
- `e54821000` lir: add restricted PHI boundary carrier
- `7d9895dfe` lir: migrate PHI boundary consumers
- `577f8ea25` docs: conclude PHI boundary retirement check

Accepted proof:

- Focused `frontend_lir_call_type_ref` proof passed after the PHI carrier
  addition.
- Matching frontend/backend guard `^frontend_|^backend_` passed with 19/19
  tests after PHI consumer migration.
- Final no-code retirement check used `git diff --check` and passed.

Delivered capability:

- Added `LirPhiOp.boundary_value_type` as a restricted PHI value-boundary
  carrier admitting only scalar, vector, aggregate, and pointer alternatives.
- Required verifier parity between the PHI carrier and compatibility
  `type_str`, with wrong-family rejection for function, void, opaque,
  runtime-text-only, and stale mirror cases.
- Migrated selected PHI verifier/modeling/printer consumers so semantic PHI
  authority comes from `boundary_value_type`; `type_str` remains only validated
  compatibility/rendering text.

Deferred work:

- Call argument/result, select, return, Raw-BIR receipt, broad receiver
  compatibility, and schema deletion remain outside this PHI-only route.
- Deleting `LirPhiOp.type_str` also remains deferred because it requires
  broader receiver/backend/schema compatibility ownership.

734 handoff:

- No direct 734 receiver handoff was accepted from this idea. A future return
  to 734 still requires one exact typed receipt row accepted by a named
  successor.

## Reviewer Reject Signals

- Reject unbounded alternatives, RTTI/vtables, missing enum cases, generic
  IDs, testcase-only handling, or retaining the universal bag behind a union.
