# RV64 Large-Offset GPR Callee-Saved Frame Slots

Status: Open
Type: RV64 frame implementation
Parent: `ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md`
Derived From: `ideas/closed/564_rv64_fpr_callee_saved_frame_slots.md`
Owning Layer: RV64 frame lowering for prepared large-offset GPR callee-saved slots

## Goal

Teach the RV64 object route to consume prepared GPR callee-saved frame-slot
facts whose stack offsets are outside the direct immediate-offset contract.

## Why This Exists

Closing the FPR callee-saved frame-slot idea moved `src/20030209-1.c` off the
old non-GPR `fpr:fs1` rejection but exposed a separate stack-frame residual:
the prepared `gpr:s1` saved slot is at `stack80000`, which is outside the
currently supported GPR saved-register offset path. This is ordinary GPR
frame-slot consumption work and should not remain folded into the completed
FPR source idea.

## Representative Rows

- `src/20030209-1.c`

Evidence:

- `build/agent_state/564_step3_fpr_callee_saved_after.log`
- Active-plan closure notes in
  `ideas/closed/564_rv64_fpr_callee_saved_frame_slots.md`

Observed diagnostic:

```text
unsupported_stack_frame: RV64 object route requires supported prepared
callee-saved save slots
```

Recorded residual fact:

```text
gpr:s1 stack80000
```

## In Scope

- Inspect and classify prepared GPR callee-saved save-slot facts with large
  stack offsets such as `gpr:s1 stack80000`.
- Add focused backend/RV64 coverage for validated large-offset GPR
  callee-saved save/restore materialization.
- Repair RV64 GPR saved-register frame lowering to use an explicit
  large-offset addressing path when prepared facts are valid.
- Preserve existing direct-offset GPR saved-register behavior.
- Keep malformed or missing prepared frame facts fail-closed.

## Out Of Scope

- FPR callee-saved save/restore materialization, which is complete under
  `ideas/closed/564_rv64_fpr_callee_saved_frame_slots.md`.
- Prepared frame-layout production or stack-slot publication.
- Generic stack-frame rewrites not required for prepared large-offset GPR
  saved-register facts.
- Instruction-fragment residuals from `src/20000603-1.c`.
- Expectation rewrites, unsupported marker changes, allowlist filtering, or
  pass/fail accounting changes.

## Acceptance Criteria

- RV64 no longer rejects the representative prepared `gpr:s1 stack80000`
  saved-register slot with the unsupported prepared callee-saved save-slot
  diagnostic.
- Tests prove large-offset GPR callee-saved save/restore materialization using
  prepared frame facts.
- Direct-offset GPR saved-register behavior remains stable.
- Missing or malformed large-offset prepared facts still fail closed.
- Any remaining representative failure has a concrete downstream owner.

## Reviewer Reject Signals

- Reject filename-shaped or register-name-only handling for `src/20030209-1.c`
  or `gpr:s1`.
- Reject changes that fabricate prepared frame slots, saved-register facts, or
  frame layout inside RV64 lowering.
- Reject broad frame rewrites or FPR changes claimed as progress for this GPR
  large-offset residual.
- Reject treating an unsupported expectation rewrite, allowlist edit, or pass
  accounting change as capability progress.
- Reject helper renames, diagnostic text churn, or classification-only edits
  that leave the large-offset GPR save-slot rejection behavior unchanged.
- Reject authorizing arbitrary memory-to-memory stack copies instead of
  consuming validated prepared callee-saved register facts.
