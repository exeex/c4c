# RV64 Prepared Callee-Saved GPR Save-Slot Emission

Status: Closed
Type: Implementation idea
Source Parent: ideas/closed/424_prepared_global_stack_frame_infrastructure_review.md
Handoff: docs/rv64_gcc_torture_post_contract/global_stack_frame_infrastructure_review.md
Owning Layer: RV64 object emission

## Goal

Teach RV64 object emission to consume coherent prepared callee-saved GPR
save-slot facts for prologue/epilogue save and restore code.

## Why This Exists

Idea 424 classified `src/20000603-1.c` as a coherent RV64 emission gap, not a
producer-contract gap. Prepared frame facts already publish fixed frame sizes,
alignments, and saved `gpr:s1` slots for both `f` and `main`, but the object
route still fails closed with:

- `RV64 object route requires supported prepared callee-saved GPR save slots`

Representative evidence:

- Row: `src/20000603-1.c`
- Bucket: `unsupported_stack_frame`
- Artifacts:
  `build/agent_state/424_step2_infrastructure_classification/20000603-1/`
- Handoff docs:
  `docs/rv64_gcc_torture_post_contract/infrastructure_bucket_evidence.md`
  and
  `docs/rv64_gcc_torture_post_contract/global_stack_frame_infrastructure_review.md`

Known prepared facts from the handoff:

- `f` has saved `gpr:s1` at `slot#10+stack16`, size/alignment `8`.
- `main` has saved `gpr:s1` at `slot#10+stack8`, size/alignment `8`.
- Frame slots and frame-slot call argument selections are explicit.

## In Scope

- RV64 object-emission support for prepared callee-saved GPR save slots whose
  saved-register facts include explicit register, slot, offset, size, and
  alignment authority.
- Prologue store and epilogue load/restore emission that consumes prepared
  facts directly.
- Focused ordinary-C backend coverage for at least one save-slot case based on
  a nearby non-testcase-shaped source pattern.
- Fail-closed diagnostics for unsupported or incomplete save-slot facts.

## Out Of Scope

- Producer changes to invent or repair saved-register facts.
- Broad FPR, F128, vector, or dynamic-stack save-slot support.
- General fixed prepared stack-frame emission beyond the frame mechanics
  required to address coherent GPR save slots.
- Expectation, unsupported-marker, allowlist, runtime-comparison, or scan
  accounting changes.

## Acceptance Criteria

- RV64 object emission lowers the representative prepared GPR save-slot route
  without reconstructing save slots from source shape or hard-coded registers.
- `src/20000603-1.c` or an equivalent focused representative advances through
  the object route for the GPR save-slot failure being repaired.
- New or updated ordinary-C coverage proves save and restore behavior for a
  nontrivial callee-saved GPR path.
- Existing unsupported diagnostics remain in place for missing, contradictory,
  FPR/F128, or otherwise unsupported save-slot facts.
- Backend proof includes the focused coverage and an appropriate `^backend_`
  subset.

## Completion Notes

Closed on 2026-07-01 after Step 5 validation completed the prepared
callee-saved GPR save-slot emission milestone.

Completed coverage:

- RV64 object emission validates explicit prepared callee-saved GPR save-slot
  facts and emits prologue stores plus epilogue restores from prepared slot
  authority.
- Focused object-emission coverage proves coherent GPR save-slot instruction
  emission and keeps unsupported non-GPR/FPR save slots fail-closed.
- Ordinary-C runtime coverage proves a nontrivial callee-saved GPR live across
  a call is preserved by the object route.
- `src/20000603-1.c` advances past the old
  `requires supported prepared callee-saved GPR save slots` diagnostic and now
  fails closed on the out-of-scope `fpr:fs1` save-slot fact.

Out-of-scope leftovers:

- FPR, F128, vector, dynamic-stack, and broad fixed-frame work remain outside
  this GPR save-slot idea and should be tracked as separate follow-up work if
  selected.
- The representative `src/20000603-1.c` row remains unsupported only because
  it has an out-of-scope FPR save-slot requirement after the GPR failure was
  repaired.

Close proof:

- Focused object-emission coverage passed with
  `ctest --test-dir build --output-on-failure -R '^backend_riscv_object_emission$'`.
- Focused runtime coverage passed with
  `ctest --test-dir build --output-on-failure -R '^backend_obj_runtime_rv64_callee_saved_gpr_live_across_call$'`.
- Existing canonical backend guard logs passed regression guard:
  `test_before.log` had `331 passed, 0 failed`, and `test_after.log` had
  `332 passed, 0 failed`.

## Reviewer Reject Signals

Reject any route or slice that:

- keys behavior on `src/20000603-1.c`, function names, raw slot numbers,
  register spellings, or one exact frame shape
- infers save slots in RV64 when the prepared contract has not published an
  explicit saved-register slot
- bundles broad FPR/F128 ABI work into this GPR save-slot idea
- changes expectations, unsupported markers, allowlists, runtime comparison,
  or pass/fail accounting instead of repairing emission
- claims progress through helper renames, diagnostics reshaping, or
  classification-only edits without object-emission capability
- leaves the old `requires supported prepared callee-saved GPR save slots`
  failure mode hidden behind a new abstraction name
