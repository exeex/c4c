# LIR Compact Scalar and ABI-Leaf Migration

Status: Open
Type: first-owner scalar nominal family migration
Matrix Rows: M1, M2
Dependencies: retained 759/760 compatibility boundaries

## Goal

Introduce a compact scalar family and migrate scalar-only schemas so vector,
aggregate, and function refs are compile-time invalid there.

## In Scope

- Add scalar store/ref and migrate scalar operation schemas, including
  separately evidenced pointer/void ABI leaves.
- Keep opaque semantics as an evidence-needed subroute; retain named
  extern/inline-asm compatibility only for named consumers.

## Out Of Scope

- Assuming opaque is scalar, aggregate/vector/function migration, or removing
  external compatibility before a native consumer exists.

## Acceptance Criteria

- Fresh build plus scalar lowering, operation verifier/printer/receiver,
  pointer/void ABI compatibility, and wrong-family proof.
- Remove scalar text classification/comparison and scalar `LirTypeRef` fields
  only after every named op/verifier/printer/receiver accepts `LirScalarRef`;
  retire each opaque factory after its named consumer migrates.

## 866 Reconciliation And 734 Return

Idea 866 selects this as the first ordered executable successor for compact
scalar and ABI-leaf producer/schema ownership. Return to 734 only when this
idea accepts an exact typed scalar or ABI-leaf handoff naming one bounded
Raw-BIR receiver row; otherwise downstream receiver work remains deferred.

## Reviewer Reject Signals

- Reject implicit cross-family conversion, text classification, opaque-policy
  guesswork, expectation downgrades, or a scalar rename retaining old fields.
