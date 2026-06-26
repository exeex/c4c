# RV64 Object Route Scalar And Floating Edge Lowering

Status: Open
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
