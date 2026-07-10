# RV64 Callee-Saved GPR Runtime

Status: Open
Type: Implementation
Parent: `ideas/open/658_backend_baseline_history_umbrella_triage.md`
Related:
- `docs/backend_baseline_history_triage/failure_classification.md`
- `docs/backend_baseline_history_triage/follow_up_order.md`
Owning Layer: RV64 callee-saved GPR preservation across calls
Queue Order: 66
Proof Surface: current baseline row 219 from
`log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`.

## Goal

Repair RV64 callee-saved GPR preservation for live values across calls.

## Why This Exists

Step 2 assigned `backend_obj_runtime_rv64_callee_saved_gpr_live_across_call`
to an RV64 prepared GPR/callee-saved runtime owner. It is a singleton row and
therefore follows broader producer/publication families.

## In Scope

- Refresh focused prepared, RV64 assembly, object, disassembly, and runtime
  evidence for the callee-saved GPR row.
- Identify whether the first owner is live-range publication, callee-saved
  slot placement, save/restore emission, call clobber modeling, or runtime
  consumption.
- Repair one general callee-saved GPR preservation rule with positive and
  fail-closed evidence.

## Out Of Scope

- Byval call-boundary payloads, pointer-local updates, object-data static
  storage, packed local member offsets, destination publication, AArch64,
  RISC-V object emission, CLI, or LLVM torture work.
- Test expectation rewrites, unsupported-marker changes, allowlists, timeout
  changes, runtime policy changes, or baseline acceptance changes.

## Acceptance Criteria

- Focused evidence names the first callee-saved GPR owner.
- The selected repair preserves live values across calls through explicit
  prepared/RV64 facts, save/restore locations, and clobber modeling.
- The focused runtime row passes or fails closed with a precise diagnostic.
- Backend regression proof shows no new backend failures in the supervisor's
  chosen subset.

## Reviewer Reject Signals

- Reject named-case fixes for the callee-saved GPR row or fixed register
  identities.
- Reject assuming a register is preserved because final assembly happens to
  work in one test without prepared/RV64 ownership facts.
- Reject expectation rewrites, unsupported-marker downgrades, allowlist edits,
  helper renames, or classification-only edits claimed as progress.
- Reject broad rewrites of call lowering, register allocation, or object
  runtime outside the callee-saved GPR preservation owner.
- Reject leaving the same live-across-call corruption behind a new diagnostic
  name.
