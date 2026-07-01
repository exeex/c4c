# RV64 Fixed Prepared Stack-Frame Emission

Status: Closed
Closed: 2026-07-01
Source Parent: ideas/open/424_prepared_global_stack_frame_infrastructure_review.md
Handoff: docs/rv64_gcc_torture_post_contract/global_stack_frame_infrastructure_review.md
Owning Layer: RV64 object emission

## Goal

Teach RV64 object emission to consume coherent prepared fixed stack-frame
plans, including large fixed frames and many fixed slots.

## Why This Exists

Idea 424 classified `src/20030209-1.c` as a coherent RV64 emission gap.
Prepared facts publish a complete fixed-frame plan, but the object route still
fails closed with:

- `RV64 object route requires a supported prepared stack frame`

Representative evidence:

- Row: `src/20030209-1.c`
- Bucket: `unsupported_stack_frame`
- Artifacts:
  `build/agent_state/424_step2_infrastructure_classification/20030209-1/`
- Handoff docs:
  `docs/rv64_gcc_torture_post_contract/infrastructure_bucket_evidence.md`
  and
  `docs/rv64_gcc_torture_post_contract/global_stack_frame_infrastructure_review.md`

Known prepared facts from the handoff:

- `frame_size=80000`
- `frame_alignment=8`
- `10000` fixed slots
- no dynamic stack
- prepared frame plan republishes the same frame

The evidence also mentions prepared FPR save slots (`fs1`, `fs2`). Those facts
are compatibility watchouts, not permission to expand this idea into broad FPR
or F128 ABI support.

## In Scope

- RV64 object-emission support for coherent fixed prepared stack-frame plans.
- Large frame-size materialization and stack-pointer adjustment when the
  immediate range requires multi-instruction materialization.
- Addressing of fixed frame slots from prepared offsets, widths, and
  alignments.
- Fail-closed handling for dynamic, contradictory, or incomplete frame plans.
- Focused ordinary-C backend coverage for a large fixed-frame path.

## Out Of Scope

- Producer changes to frame layout, slot numbering, or ABI homes.
- Dynamic stack allocation, VLAs, alloca, varargs, F128, or broad FPR ABI
  support.
- Callee-saved GPR save-slot support except where a separate save-slot route
  has already made those facts consumable.
- Expectation, unsupported-marker, allowlist, runtime-comparison, or scan
  accounting changes.

## Acceptance Criteria

- RV64 object emission consumes prepared fixed-frame size, alignment, and slot
  facts directly for a large-frame representative.
- `src/20030209-1.c` or a nearby focused representative advances past the
  generic prepared-stack-frame rejection for the fixed-frame capability being
  repaired.
- Ordinary-C coverage proves fixed-slot addressing for a large frame without
  relying on testcase names or row IDs.
- Unsupported or incomplete frame-plan variants still fail closed.
- Backend proof includes the focused coverage and an appropriate `^backend_`
  subset.

## Completion Notes

Closed after Steps 3-6 completed the fixed prepared stack-frame consumer
repair for RV64 object emission.

The completed route consumes explicit prepared fixed-frame size, alignment,
slot offset, width, and alignment facts for coherent fixed frames. Large frame
adjustments and large fixed-slot addresses are materialized from those prepared
facts, and the new ordinary-C runtime coverage proves a large fixed-frame
fixed-slot path without relying on the gcc torture row name, `80000`, `10000`,
or one exact slot layout.

The representative `tests/c/external/gcc_torture/src/20030209-1.c` now
advances past the old fixed-frame rejection:

- `RV64 object route requires a supported prepared stack frame`

It remains fail-closed on the existing out-of-scope saved-register ABI
boundary:

- `unsupported_stack_frame: RV64 object route does not support non-GPR prepared
  callee-saved register save slots (fpr:fs1)`

That FPR saved-register boundary, plus FPR/F128/vector, dynamic stack,
producer frame-layout gaps, and broader ABI support, remains outside this
source idea and should be handled as follow-up work if pursued.

Close evidence:

- `backend_riscv_object_emission` passed, including focused fixed-frame object
  emission coverage and fail-closed object-shape checks.
- `backend_obj_runtime_rv64_large_fixed_frame_slot_access` passed, proving the
  ordinary-C large fixed-frame fixed-slot runtime route.
- Focused representative proof for `src/20030209-1.c` showed the old
  fixed-frame rejection absent and the residual `fpr:fs1` diagnostic present.
- Close-time regression guard passed using matching canonical backend logs:
  `test_before.log` reported 331/331 passing tests, `test_after.log` reported
  333/333 passing tests, and
  `.codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py`
  reported no new failures.

## Reviewer Reject Signals

Reject any route or slice that:

- special-cases `src/20030209-1.c`, `80000`, `10000`, or one exact slot layout
- reconstructs frame size, stack offsets, widths, or alignments in RV64 from
  source shape instead of consuming prepared facts
- mixes producer frame-layout repair into this RV64 consumer idea
- expands the slice into F128, broad FPR ABI, dynamic-stack, or unrelated
  runtime-mismatch work
- changes expectations, unsupported markers, allowlists, runtime comparison,
  or pass/fail accounting instead of adding emission capability
- leaves the old `requires a supported prepared stack frame` failure mode
  behind a renamed helper or broader diagnostic
