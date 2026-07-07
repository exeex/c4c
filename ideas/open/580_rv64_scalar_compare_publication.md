# RV64 Scalar Compare Publication

Status: Open
Type: Focused RV64 object-emission repair
Parent: `ideas/open/550_rv64_scalar_fpr_residual_salvage.md`
Owning Layer: RV64 object scalar compare result publication

## Goal

Teach the RV64 object route to publish non-terminator FPR compare results into
their prepared scalar homes without relying on testcase-specific fallbacks.

## Why This Exists

Step 3 of the scalar/FPR residual salvage lane classified four current rows
under one semantic owner: RV64 object emission cannot publish ordinary FPR
compare results when the prepared result home is a GPR. The current evidence is
saved in:

- `build/agent_state/550_rv64_scalar_fpr_residual_salvage/step3/owners.tsv`
- `build/agent_state/550_rv64_scalar_fpr_residual_salvage/step3/summary.md`

Rows:

- `src/20080529-1.c`: `bir.ne float ...`, result home `t0`
- `src/930818-1.c`: `bir.eq double ...`, result home `t0`
- `src/loop-8.c`: `bir.ne double ...`, result home `s1`
- `src/strct-pack-1.c`: `bir.ne double ...`, result home `s1`

`src/loop-8.c` and `src/strct-pack-1.c` also exercise select-chain or
select-carrier publication paths, but Step 3 records the first owner as scalar
compare result publication.

## In Scope

- RV64 object-emission lowering/publication for ordinary F32/F64 equality and
  inequality compare results.
- Prepared GPR result homes for non-terminator compare instructions.
- Focused tests that cover both direct compare result publication and
  compare-result publication feeding select/materialization paths.
- Representative route proof for at least one simple compare row and one
  select-consuming compare row.

## Out Of Scope

- F128, long-double, external soft-float helper, or libc helper work.
- Floating cast lowering; that is tracked separately.
- Variadic helper lowering; that is tracked separately.
- Broad FPR ownership rewrites not required to publish scalar compare results.
- Expected-output rewrites or unsupported-marker changes as a substitute for
  capability repair.

## Acceptance Criteria

- Focused RV64 object-emission coverage proves non-terminator FPR compare
  results can be published into prepared GPR homes.
- At least one current scalar-compare residual row advances past the old
  `unsupported_scalar_compare_publication` owner.
- Select-consuming compare rows are either proven to advance or left with a
  later, narrower owner that is not the original compare-publication failure.
- Backend validation for the touched RV64 bucket passes.

## Reviewer Reject Signals

- Reject named-case checks for `20080529-1.c`, `930818-1.c`, `loop-8.c`, or
  `strct-pack-1.c` instead of semantic compare lowering/publication.
- Reject changing unsupported expectations, route allowlists, or failure bucket
  labels and claiming that as compare-publication progress.
- Reject a fix that only handles one compare opcode or one hard-coded register
  when nearby F32/F64 eq/ne compare result homes still fail the same way.
- Reject mixing this slice with floating cast or variadic helper work unless a
  reviewer has separate evidence that the implementation owner truly changed.
- Reject broad FPR or select rewrites that leave the original
  `unsupported_scalar_compare_publication` mode intact behind a new helper
  name.
