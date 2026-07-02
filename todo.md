Status: Active
Source Idea Path: ideas/open/548_prepared_global_stack_frame_infrastructure_review.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Global-Data Ownership

# Current Packet

## Just Finished

Completed plan Step 2 by classifying the two current
`unsupported_global_data` representatives into prepared-contract versus RV64
object-route ownership.

Classifications:

- `src/20000412-1.c`:
  first owner is the prepared object-data contract. The row reports
  `prepared selected object-data contract status=unsupported_but_coherent
  object_label_id=2 object_size_bytes=1656 emitted_byte_count=0
  zero_fill_byte_count=0`. Prepared has a label and extent, but it has not
  published consumable zero-fill/emitted-byte authority for the selected global
  object data. RV64 sees selected object data but cannot consume
  `unsupported_but_coherent`, and the selected zero-fill fallback does not
  apply because the facts report no zero-fill authority for the 1656-byte
  object.
- `src/20001121-1.c`:
  first owner is RV64 object-route global-data consumption. Prepared facts are
  present far enough for the route to classify the operation as prepared global
  memory access; the failing gate is RV64's
  `rv64_global_scalar_memory_size_for_type`, which currently accepts
  integer/pointer `I8`, `I16`, `I32`, `I64`, and `Ptr`, but not the row's
  global `double` / `F64` load shape.

Implementation-ready follow-up buckets:

- Prepared producer follow-up: publish coherent selected object-data facts for
  ordinary zero-initialized global arrays like `src/20000412-1.c`, rather than
  teaching RV64 to consume unsupported-marker selected object data.
- RV64 consumer follow-up: add or explicitly route prepared floating global
  load/store consumption for non-F128 `double` globals like `src/20001121-1.c`.

Evidence artifact:

- `build/agent_state/548_step2_global_data_classification/classification.md`

## Suggested Next

Ask plan-owner to advance to Step 3 so the stack-frame representatives
`src/20000603-1.c` and `src/20030209-1.c` can be classified for prepared-frame
contract versus RV64 object-route ownership.

## Watchouts

- This is a review/classification plan. Do not implement prepared or RV64
  lowering inside this active idea.
- `src/20000412-1.c` and `src/20001121-1.c` should split into separate
  follow-up ideas if implementation proceeds: prepared zero-fill object-data
  producer repair versus RV64 floating global-memory consumption.
- No F128 quarantine was discovered for either global-data row. `20000412-1.c`
  is an ordinary pointer-array global case; `20001121-1.c` uses `double`, not
  F128.
- The current stack-frame evidence is FPR callee-saved slot shaped
  (`fpr:fs1`), so Step 3 should explicitly screen FPR scope before treating it
  as ordinary GPR frame work.
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
  `build/rv64_gcc_c_torture_backend/src_20000412-1.c/case.log`
- Existing per-case log:
  `build/rv64_gcc_c_torture_backend/src_20001121-1.c/case.log`
- Focused Step 2 classification artifact:
  `build/agent_state/548_step2_global_data_classification/classification.md`

No broad tests were run and no root-level `test_after.log` was written because
the delegated packet explicitly requested existing-log classification only and
forbade root-level proof logs.

Traceable logs:

- Aggregate:
  `build/agent_state/548_step1_infrastructure_evidence.log`
- Per-case:
  `build/rv64_gcc_c_torture_backend/src_20000412-1.c/case.log`
- Per-case:
  `build/rv64_gcc_c_torture_backend/src_20001121-1.c/case.log`
