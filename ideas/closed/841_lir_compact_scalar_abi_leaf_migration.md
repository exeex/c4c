# LIR Compact Scalar and ABI-Leaf Migration

Status: Closed
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

## Closure Note

Disposition: closed as a bounded capability slice, not as terminal scalar or
ABI-leaf family completion.

Accepted commits:

- `da9fdd291` - selected the compact scalar binary target and excluded wrong
  families.
- `8198c5f7b` - added `LirCompactScalarType` and attached it to `LirBinOp` as
  optional compact scalar authority derived from `type_str`.
- `e89fa5ffe` - migrated binop verifier/printer scalar consumers to the compact
  authority while preserving rendering parity.
- `0645ae3c7` - retired the modeled-result binop `type_str` source in favor of
  `LirBinOp.compact_scalar_type` when present.

Accepted proof:

- Step proofs passed with fresh builds and focused frontend coverage, ending
  with `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_'; } > test_after.log 2>&1`.
- Supervisor accepted a full baseline candidate after `0645ae3c7`:
  `test_baseline.new.log` was 3038/3038, then
  `scripts/plan_review_state.py accept-baseline` was run.
- Supervisor handled the Step 4 code-review reminder directly and found no
  blocking issues in the bounded diff.

Exact capability delivered:

- The selected scalar integer/floating `LirBinOp` row now has a compact scalar
  carrier/ref with verifier coverage for valid scalar positives and
  vector/aggregate/function/opaque/pointer/void wrong-family rejection.
- The ordinary modeled-result path now treats `LirBinOp.compact_scalar_type` as
  selected scalar result authority when present; `LirBinOp.type_str` remains as
  compatibility/rendering parity text and fallback for unmigrated/manual
  construction.

Deferred work and return condition:

- This closure does not implement a 734 Raw-BIR receiver packet and records no
  exact accepted typed handoff to 734.
- Nonselected scalar schemas, pointer/void ABI-leaf authority, opaque policy,
  backend lowering, Raw-BIR importer/container work, and terminal text-field
  deletion remain deferred to their own exact successors.
- Per the 866 ordering, continue with
  `ideas/open/842_lir_restricted_first_class_value_unions.md` next. Later
  receiver work may return to 734 only after a successor accepts one exact
  typed scalar or ABI-leaf handoff naming a bounded Raw-BIR receiver row.
