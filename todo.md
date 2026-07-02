Status: Active
Source Idea Path: ideas/open/548_prepared_global_stack_frame_infrastructure_review.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Split Follow-Up Infrastructure Ideas

# Current Packet

## Just Finished

Completed plan Step 4 by classifying the prepared move-bundle bucket.
Together with Steps 2 and 3, all three infrastructure buckets now have
current first-owner evidence:

- Global data:
  `src/20000412-1.c` is prepared object-data contract work for zero-fill or
  coherent object-data authority; `src/20001121-1.c` is RV64 global-memory
  consumption work for non-F128 `double` / `F64` global loads.
- Stack frame:
  `src/20000603-1.c` and `src/20030209-1.c` are RV64 frame-lowering work for
  prepared non-GPR callee-saved save slots, specifically `fpr:fs1`, not
  ordinary GPR frame evidence gaps.
- Prepared move bundle:
  `src/20010224-1.c` and `src/pr87623.c` remain prepared classifier authority
  work for conversion-adjacent stack-slot to stack-slot integer moves with
  differing source/destination widths and `authority=none`.

## Suggested Next

Create focused follow-up ideas under `ideas/open/` from the classified buckets,
then close or retire this review plan once those durable implementation lanes
exist.

Recommended follow-up ideas:

- Prepared object-data zero-fill contract:
  use `src/20000412-1.c` as the representative. Acceptance should require
  prepared/BIR object data to carry coherent zero-fill authority for the
  1656-byte global object without expectation downgrades.
- RV64 global-memory scalar consumption:
  use `src/20001121-1.c` as the representative. Acceptance should require RV64
  object-route global-memory support for non-F128 `double` / `F64` loads after
  prepared facts already exist.
- RV64 FPR callee-saved frame slots:
  use `src/20000603-1.c` and `src/20030209-1.c` as representatives.
  Acceptance should require RV64 frame setup/access support for prepared
  non-GPR callee-saved slot `fpr:fs1` while keeping ordinary GPR frame work
  out unless separately evidenced.
- Prepared move-bundle conversion authority:
  use `src/20010224-1.c` and `src/pr87623.c` as representatives. Acceptance
  should require the prepared classifier to publish authority for the
  conversion-adjacent stack-slot to stack-slot integer move shape, or to split
  it into an explicit supported prepared form before RV64 consumption.

Recommended lifecycle decision: split these implementation-ready buckets into
separate source ideas, then close this infrastructure review idea as complete
classification work. If the supervisor wants fewer immediate ideas, create the
prepared move-bundle and RV64 global-memory ideas first because they have the
clearest single-owner evidence.

## Watchouts

- This is a review/classification plan. Do not implement prepared or RV64
  lowering inside this active idea.
- Step 5 should create durable source ideas only; it should not claim
  capability progress from classification artifacts.
- `src/20000412-1.c` and `src/20001121-1.c` should split into separate
  follow-up ideas if implementation proceeds: prepared zero-fill object-data
  producer repair versus RV64 floating global-memory consumption.
- No F128 quarantine was discovered for either global-data row. `20000412-1.c`
  is an ordinary pointer-array global case; `20001121-1.c` uses `double`, not
  F128.
- Stack-frame implementation work should split into an RV64 FPR
  callee-saved-frame follow-up if accepted. Keep ordinary GPR frame work out of
  that scope unless separate evidence identifies a GPR-specific gap.
- The move-bundle rows still name `prepared_move_bundle_classifier` as
  diagnostic owner and `authority=none`; avoid converting them into RV64
  consumer work before the prepared classifier authority gap is repaired.
- Do not change expectations, unsupported markers, allowlists, or pass/fail
  accounting as evidence of progress.

## Proof

Proof/evidence used:

- Step 2 global-data classification:
  `build/agent_state/548_step2_global_data_classification/classification.md`
- Step 3 stack-frame classification:
  `build/agent_state/548_step3_stack_frame_classification/classification.md`
- Step 4 move-bundle classification:
  `build/agent_state/548_step4_move_bundle_classification/classification.md`
- Existing Step 1 aggregate log:
  `build/agent_state/548_step1_infrastructure_evidence.log`

No broad tests were run and no root-level `test_after.log` was written for
this lifecycle advance; it only updates `todo.md` to point at Step 5.

Traceable logs:

- Aggregate:
  `build/agent_state/548_step1_infrastructure_evidence.log`
- Per-case:
  `build/rv64_gcc_c_torture_backend/src_20010224-1.c/case.log`
- Per-case:
  `build/rv64_gcc_c_torture_backend/src_pr87623.c/case.log`
