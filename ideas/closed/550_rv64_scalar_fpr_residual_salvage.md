# RV64 Scalar/FPR Residual Salvage

Status: Closed
Type: Low-volume ordinary-C salvage plan
Parent: `ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md`
Owning Layer: RV64 scalar/FPR/helper boundary

## Goal

Classify and sequence low-volume scalar/FPR residual failures without letting
them displace higher-impact ordinary-C buckets or merge with F128 work.

## Why This Exists

The current bucket map records small explicit residuals: 3
`unsupported_scalar_compare_publication` rows, 2 `unsupported_floating_cast`
rows, and 1 `unsupported_variadic_helper_lowering` row. These rows may be
useful salvage work after higher-count buckets, but they are not the main RV64
recovery route.

## In Scope

- Reproduce the six current residual rows and classify first ownership.
- Separate scalar compare/publication, scalar F32/F64 cast, and variadic helper
  ABI work.
- Create narrow follow-up ideas only when a residual group has coherent facts
  and useful proof scope.
- Keep primary-F128 or long-double helper rows in the F128 quarantine lane.

## Out Of Scope

- F128 arithmetic, F128 conversion, or external soft-float implementation.
- Broad FPR rewrites not justified by current non-F128 residual rows.
- Treating low-volume salvage as higher priority than move-bundle,
  instruction-fragment, local-memory, global-data, or stack-frame work.
- Changing expected outputs, unsupported markers, or pass/fail accounting.

## Acceptance Criteria

- Each residual row is classified with first owner and F128 screening.
- Any implementation follow-up is narrow, non-F128, and backed by current row
  evidence.
- Variadic helper work is kept separate from scalar compare and FPR cast work.
- The idea records why no immediate implementation is needed if row evidence
  does not justify it.

## Closure Note

Closed after the classification/splitting lane completed. Current evidence
found ten direct residual matches rather than the older six-row source count:
four scalar compare publication rows, five floating-cast rows, and one
variadic helper row. Step 2 quarantined `src/20040709-1.c` and
`src/ieee/20011123-1.c` as F128/long-double work, leaving seven ordinary
scalar/FPR rows and one variadic helper row for ownership classification.

Step 3 classified the retained non-quarantined rows into three separate
semantic owners with no shared implementation owner: scalar compare
publication, ordinary F32/F64 floating cast lowering, and `va_start`
stack-backed destination lowering. Step 4 split those actionable groups into
separate open follow-up ideas:

- `ideas/open/580_rv64_scalar_compare_publication.md`
- `ideas/open/581_rv64_ordinary_floating_cast_lowering.md`
- `ideas/open/582_rv64_va_start_stack_backed_destination.md`

No implementation was performed under this classification source idea.

## Reviewer Reject Signals

- Reject using scalar/FPR residual work to continue the F128 `conversion.c`
  route.
- Reject helper renames, expectation rewrites, or unsupported downgrades as
  scalar/FPR progress.
- Reject width-specific or case-specific lowering shortcuts not grounded in
  semantic type facts.
- Reject broad FPR or call ABI rewrites from only the six current residual
  rows.
- Reject mixing variadic helper, scalar compare, and floating-cast work into a
  single implementation slice without proof they share one semantic owner.
