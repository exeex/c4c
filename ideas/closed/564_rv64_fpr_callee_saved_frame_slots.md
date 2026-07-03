# RV64 FPR Callee-Saved Frame Slots

Status: Closed
Type: RV64 frame implementation
Parent: `ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md`
Derived From: `ideas/closed/548_prepared_global_stack_frame_infrastructure_review.md`
Owning Layer: RV64 frame lowering for prepared FPR callee-saved slots

## Goal

Teach the RV64 object route to consume prepared non-GPR callee-saved frame
slots for FPR saves/restores, starting with the `fpr:fs1` save-slot shape.

## Why This Exists

Step 3 of the prepared infrastructure review classified both stack-frame
representatives as RV64 frame-lowering work. Prepared callee-saved frame facts
are present enough for RV64 to inspect `fpr:fs1`, but target object emission
rejects non-GPR prepared callee-saved save slots.

## Representative Rows

- `src/20000603-1.c`
- `src/20030209-1.c`

Evidence:

- `build/agent_state/548_step3_stack_frame_classification/classification.md`
- `build/rv64_gcc_c_torture_backend/src_20000603-1.c/case.log`
- `build/rv64_gcc_c_torture_backend/src_20030209-1.c/case.log`

Observed diagnostic:

```text
prepared module shape: unsupported_stack_frame: RV64 object route does not
support non-GPR prepared callee-saved register save slots (fpr:fs1)
```

## In Scope

- Implement RV64 frame setup/access/teardown support for prepared FPR
  callee-saved save slots proven by the representative `fpr:fs1` shape.
- Add focused target/backend coverage for non-GPR prepared callee-saved slot
  consumption.
- Preserve prepared frame fact ownership; RV64 should consume slots that are
  already published, not synthesize missing frame layout.
- Re-run both representative rows and classify any later residual owner.

## Out Of Scope

- Ordinary GPR stack-frame work unless separate evidence identifies a GPR gap.
- Prepared frame-layout production or stack-slot publication repair.
- Global-data object route work.
- F128 quarantine or soft-float policy work unless the representative proves a
  direct non-GPR callee-saved slot dependency.
- Expectation rewrites, unsupported marker changes, or pass/fail accounting.

## Acceptance Criteria

- RV64 no longer rejects prepared `fpr:fs1` callee-saved save slots with the
  current non-GPR stack-frame diagnostic.
- Tests prove FPR callee-saved frame-slot consumption using prepared facts.
- The implementation keeps GPR frame behavior stable and does not widen scope
  without fresh row evidence.
- Remaining representative failures, if any, have a new concrete owner.

## Closure Notes

Closed on 2026-07-03 after `d57660794 materialize FPR callee-saved frame
slots`.

Accepted evidence:

- `6286dc3cd [todo_only] record FPR callee-saved boundary`
- `8d04a28ce [todo_only] advance FPR frame-slot coverage state`
- `1867cff54 cover prepared FPR callee-saved slots`
- `606eb255a [todo_only] advance FPR callee-saved repair state`
- `d57660794 materialize FPR callee-saved frame slots`

Close proof:

- Backend proof passed in `test_after.log`: `345/345`.
- Regression guard passed against `test_before.log`: before `345/0`, after
  `345/0`.
- Representative probe for `src/20000603-1.c` and `src/20030209-1.c` no
  longer contains the old non-GPR prepared callee-saved register save-slot
  diagnostic for `fpr:fs1`.

Residuals are outside this source idea:

- `src/20000603-1.c` now fails later at
  `unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering`.
- `src/20030209-1.c` has a distinct large-offset GPR saved-register frame-slot
  residual for prepared `gpr:s1` at `stack80000`.

The large-offset GPR residual is tracked separately in
`ideas/open/566_rv64_large_offset_gpr_callee_saved_frame_slots.md`.

## Reviewer Reject Signals

- Reject changes that fabricate prepared frame slots, callee-saved facts, or
  frame layout in RV64 lowering.
- Reject broad GPR frame rewrites justified only by the FPR `fs1` evidence.
- Reject filename-shaped handling for `src/20000603-1.c` or
  `src/20030209-1.c`, register-name-only shortcuts, or diagnostic filtering.
- Reject expectation rewrites, unsupported downgrades, allowlist filtering, or
  pass/fail accounting changes as progress.
- Reject helper renames or classification-only edits claimed as FPR frame
  support.
- Reject retaining the same non-GPR callee-saved slot rejection behind a new
  abstraction name.
