# RV64 Ordinary Floating Cast Lowering

Status: Closed
Type: Focused RV64 object-emission repair
Parent: `ideas/open/550_rv64_scalar_fpr_residual_salvage.md`
Owning Layer: RV64 object `CastInst` lowering for ordinary F32/F64 forms

## Goal

Implement or narrow RV64 object lowering for ordinary non-F128 floating cast
forms identified by the current residual rows.

## Why This Exists

Step 3 of the scalar/FPR residual salvage lane classified three retained rows
under RV64 object `CastInst` lowering for ordinary F32/F64 forms, after Step 2
quarantined F128 and long-double rows away from this lane. Evidence is saved in:

- `build/agent_state/550_rv64_scalar_fpr_residual_salvage/step2/summary.md`
- `build/agent_state/550_rv64_scalar_fpr_residual_salvage/step3/owners.tsv`
- `build/agent_state/550_rv64_scalar_fpr_residual_salvage/step3/summary.md`

Rows:

- `src/920618-1.c`: `bir.fptrunc double constant to float`
- `src/ieee/pr67218.c`: `bir.uitofp i32 ... to float` followed by
  `bir.fpext float ... to double`
- `src/pr23941.c`: ordinary F64/F32 truncation and F32-to-F64 extension

Step 2 quarantined `src/20040709-1.c` and `src/ieee/20011123-1.c` as
F128/long-double work; they must not justify this idea's implementation.

## In Scope

- RV64 object-emission lowering for ordinary F32/F64 `fptrunc` and `fpext`.
- Ordinary integer-to-FP cast forms needed by the retained `uitofp i32 to
  float` evidence.
- Focused tests that prove semantic cast lowering without matching row names.
- Representative route proof over the retained ordinary floating-cast rows.

## Out Of Scope

- F128, long-double, external soft-float helpers, and F128 carrier plumbing.
- Scalar compare publication; that is tracked separately.
- Variadic `va_start` helper lowering; that is tracked separately.
- Full cast-matrix completion beyond the ordinary F32/F64 and integer-to-F32
  forms justified by current evidence.
- Expectation rewrites or unsupported-marker changes as progress.

## Acceptance Criteria

- Focused RV64 object-emission coverage proves the selected ordinary cast forms
  are lowered or rejected with narrower semantic diagnostics.
- At least one retained floating-cast representative route advances past the
  old `unsupported_floating_cast` owner.
- Quarantined F128/long-double rows remain outside the proof and are not used
  to claim this idea complete.
- Backend validation for the touched RV64 bucket passes.

## Closure Notes

Closed after the active runbook completed all acceptance criteria.

- Focused RV64 object-emission coverage now proves ordinary F32/F64 width casts,
  including immediate-source `FPTrunc F64 -> F32`, immediate-source
  `FPExt F32 -> F64`, and the retained `UIToFP i32 -> F32` plus
  `FPExt F32 -> F64` chain.
- Implementation uses semantic opcode/type/home lowering rather than retained
  row names.
- Representative route proof for `src/920618-1.c`, `src/ieee/pr67218.c`, and
  `src/pr23941.c` advanced all three retained ordinary cast rows past
  `unsupported_floating_cast` to downstream `unsupported_terminator_fragment`.
- F128, long-double, soft-float helper, scalar compare, and variadic helper rows
  stayed outside the proof claim.
- Backend close gate passed with matching `^backend_` before/after logs.

The downstream `unsupported_terminator_fragment` owner is a separate follow-up
candidate and is not part of this ordinary floating-cast lowering idea.

## Reviewer Reject Signals

- Reject using `src/20040709-1.c`, `src/ieee/20011123-1.c`, F128 carriers, or
  long-double helpers as implementation justification for this idea.
- Reject hard-coded handling for `920618-1.c`, `pr67218.c`, or `pr23941.c`
  instead of type/opcode-driven cast lowering.
- Reject changes that only rename `unsupported_floating_cast` or weaken route
  expectations without advancing or narrowing the first semantic owner.
- Reject broad cast rewrites that skip focused proof for `fptrunc`, `fpext`,
  and `uitofp` shapes present in the evidence.
- Reject mixing this slice with scalar compare publication or variadic helper
  work unless new evidence proves a shared owner.
