# LIR-To-New-BIR VLA Stack-Restore Receiver Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Resumed from: closed 794 selected VLA stack-restore authority handoff.

## Purpose

Receive exactly closed 794's selected VLA `LirStackRestoreOp` authority in
typed Raw-BIR, without repeating accepted receiver work or widening the
local/VLA surface.

## Historical Progress

Steps 1 through 7.30 are accepted historical 734 receiver work. Closed 794
completed only the selected producer-side handoff; do not redo either the
earlier receiver steps or its producer authority work.

## Core Rule

Use only the checked native fields in
`docs/lir_local_operation_authority/handoff_to_734.md`. Do not infer value,
object, owner, type, liveness, row identity, or lifetime transition from names,
formatted operands, LLVM text, testcase identity, `monostate`, or an unresolved
classification.

## Read First

- `docs/lir_local_operation_authority/handoff_to_734.md`
- `ideas/closed/794_lir_next_local_vla_authority_handoff.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md` (selected
  stack-restore resumption record)

## Non-Goals

- dynamic-VLA count/allocation, VLA GEP, other local rows, broad local/VLA
  conversion, a generic lifetime-model redesign, or multiple row admissions;
- target lowering, MIR, emission, and any work beyond this one typed Raw-BIR
  receiver packet;
- presentation-derived authority or weaker verifier/test contracts.

## Ordered Steps

### Step 7.31 - Receive the selected VLA LirStackRestoreOp authority

Goal: transactionally import closed 794's one selected stack-restore row into
a typed Raw-BIR destination.

Actions:

- map only selected admission, `saved_ptr`, matching pointer definition,
  current-function object/owner, pointer-type/pointee-type/live checkpoint
  binding, and `RestoreSavedVlaStackCheckpoint` transition;
- add only the minimum typed destination, importer dispatch, reachable
  verification, and nearby positive/negative receiver coverage;
- reject every malformed or unselected form before publication and run a fresh
  build plus focused receiver proof before supervisor-selected broader proof.

Completion check: exactly the selected stack restore imports and verifies
transactionally without presentation recovery; dynamic-VLA allocation/count,
VLA GEP, all other local/lifetime rows, and later families remain fail closed.
