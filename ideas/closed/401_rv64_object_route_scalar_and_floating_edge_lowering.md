# RV64 Object Route Scalar And Floating Edge Lowering

Status: Closed
Type: Follow-up repair idea
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`

## Goal

Repair current RV64 object-route scalar compare/trunc and floating-cast edge
lowering failures.

## Why This Exists

The 2026-06-26 reopened 354 classification found 42 scalar/floating edge
failures:

- 36 `unsupported_scalar_compare_trunc`
- 6 `unsupported_floating_cast`

Representatives:

- `tests/c/external/gcc_torture/src/20000313-1.c`
- `tests/c/external/gcc_torture/src/20020225-2.c`
- `tests/c/external/gcc_torture/src/20000403-1.c` after the separate `I16`
  formal ABI publication repair

## In Scope

- Generalize scalar compare result publication and integer truncation beyond
  the currently admitted narrow named shape.
- Add RV64 FPR register floating cast lowering for prepared F32/F64 forms
  observed in the bucket.
- If the failure is caused by missing or incorrect BIR/prepared scalar,
  truncation, cast, or FPR publication facts, split that producer repair into
  a separate BIR/prepared idea instead of compensating in MIR/RV64 emission.
- Prove representative and nearby scalar/floating cases through qemu
  comparison.

## Out Of Scope

- Rewriting unrelated scalar-binop semantic `lir_to_bir` failures.
- Implementing all floating-point ABI work in one slice unless required by
  the observed prepared forms.
- Hard-coding compare names or testcase constants.

## Acceptance

- `src/20000313-1.c` no longer fails with
  `unsupported_scalar_compare_trunc`.
- `src/20020225-2.c` no longer fails with `unsupported_floating_cast`, or the
  remaining blocker is split into a precise follow-up.
- A refreshed subset shows this bucket count decreases without new runtime
  mismatches.

## Reviewer Reject Signals

- Reject hard-coded compare/trunc sequences that only match one testcase.
- Reject clearing the diagnostic by widening or dropping truncation semantics.
- Reject floating casts that compile but produce ABI-incoherent FPR/GPR moves.
- Reject MIR/RV64 changes that invent scalar result, truncation, or floating
  cast facts that should have been represented by BIR/prepared lowering.
- Reject expectation rewrites or allowlist filtering.

## Completion Notes

- 2026-06-26: `src/20000313-1.c` and `src/20000403-1.c` moved past
  `unsupported_scalar_compare_trunc` after reusable scalar compare publication
  and predicate-normalization lowering.
- 2026-06-26: `src/compare-2.c` moved to `pass`; its blocker was ordinary
  select predicate lowering, not scalar compare/trunc publication.
- 2026-06-26: `src/20020225-2.c` moved past `unsupported_floating_cast` after
  reusable RV64 `SIToFP`/`UIToFP` lowering from integer homes into prepared FPR
  destinations.
- 2026-06-26: The remaining `src/20020225-2.c` blocker is now
  `unsupported_local_memory_access` for local aggregate/union frame-slot
  addressing and belongs to
  `ideas/open/400_rv64_object_route_local_memory_addressing_edges.md`.
- 2026-06-26: `src/int-compare.c` exposed a separate wide rematerialized
  immediate producer-admission boundary and was split to
  `ideas/open/404_prepared_wide_rematerialized_immediate_admission.md`.
- 2026-06-26: `src/pr48973-2.c` exposed scalar `ashr` after global bitfield
  load and remains in the existing instruction-fragment lowering family,
  `ideas/open/395_rv64_object_route_instruction_fragment_lowering.md`.
- 2026-06-26: Close gate accepted with backend CTest regression guard:
  `test_before.log` passed 326/326, regenerated `test_after.log` passed
  327/327, no new failures.
