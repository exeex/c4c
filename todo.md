Status: Active
Source Idea Path: ideas/open/548_prepared_global_stack_frame_infrastructure_review.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Classify Prepared Move-Bundle Ownership

# Current Packet

## Just Finished

Completed plan Step 3 by classifying the two current
`unsupported_stack_frame` representatives into prepared-frame contract versus
RV64 frame-lowering ownership.

Classifications:

- `src/20000603-1.c`:
  first owner is RV64 frame lowering for prepared non-GPR callee-saved save
  slots. Prepared frame/callee-saved facts are present enough for RV64 object
  emission to inspect a saved callee register with bank `fpr` and register
  name `fs1`; the first rejection is the RV64 consumer gate
  `diagnose_unsupported_prepared_saved_register_bank`, which currently accepts
  only GPR prepared callee-saved save slots.
- `src/20030209-1.c`:
  same first owner as `src/20000603-1.c`. The row reaches the same prepared
  FPR callee-saved slot shape and fails at the same RV64 object-route consumer
  gate: `unsupported_stack_frame: RV64 object route does not support non-GPR
  prepared callee-saved register save slots (fpr:fs1)`.

Scope classification:

- Prepared frame facts are not the first missing contract for either row; the
  diagnostic proves the object route has a prepared callee-saved slot to
  inspect.
- The `fpr:fs1` shape is FPR-specific. Treat it as an implementation-ready
  RV64 FPR frame follow-up only if the follow-up is scoped to FPR
  callee-saved save/restore plus its prepared slot placement contract.
- Ordinary GPR frame setup, access, and teardown should stay separate; these
  rows do not prove a retained GPR-frame producer or consumer gap.

Evidence artifact:

- `build/agent_state/548_step3_stack_frame_classification/classification.md`

## Suggested Next

Classify the prepared move-bundle representatives `src/20010224-1.c` and
`src/pr87623.c` from the existing Step 1 evidence. Confirm whether each row
still stops at `prepared_move_bundle_classifier` authority, has a missing
prepared source/destination form, should remain fail-closed, or has enough
prepared move-bundle facts to hand off to RV64 object-route consumption.

## Watchouts

- This is a review/classification plan. Do not implement prepared or RV64
  lowering inside this active idea.
- `src/20000412-1.c` and `src/20001121-1.c` should split into separate
  follow-up ideas if implementation proceeds: prepared zero-fill object-data
  producer repair versus RV64 floating global-memory consumption.
- No F128 quarantine was discovered for either global-data row. `20000412-1.c`
  is an ordinary pointer-array global case; `20001121-1.c` uses `double`, not
  F128.
- Stack-frame implementation work should split into an RV64 FPR
  callee-saved-frame follow-up if accepted. Keep ordinary GPR frame work out of
  that scope unless separate evidence identifies a GPR-specific gap.
- The move-bundle rows already name `prepared_move_bundle_classifier` as
  diagnostic owner; avoid converting them into RV64 consumer work before
  classifier authority is reviewed.
- Do not change expectations, unsupported markers, allowlists, or pass/fail
  accounting as evidence of progress.

## Proof

Proof/evidence used:

- Existing Step 1 aggregate log:
  `build/agent_state/548_step1_infrastructure_evidence.log`
- Existing per-case log:
  `build/rv64_gcc_c_torture_backend/src_20000603-1.c/case.log`
- Existing per-case log:
  `build/rv64_gcc_c_torture_backend/src_20030209-1.c/case.log`
- Focused Step 3 classification artifact:
  `build/agent_state/548_step3_stack_frame_classification/classification.md`

No broad tests were run and no root-level `test_after.log` was written because
the delegated packet explicitly requested existing-log classification only and
forbade root-level proof logs.

Traceable logs:

- Aggregate:
  `build/agent_state/548_step1_infrastructure_evidence.log`
- Per-case:
  `build/rv64_gcc_c_torture_backend/src_20000603-1.c/case.log`
- Per-case:
  `build/rv64_gcc_c_torture_backend/src_20030209-1.c/case.log`
