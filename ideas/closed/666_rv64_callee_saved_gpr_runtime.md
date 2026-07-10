# RV64 Callee-Saved GPR Runtime

Status: Closed
Type: Implementation
Parent: `ideas/open/658_backend_baseline_history_umbrella_triage.md`
Related:
- `docs/backend_baseline_history_triage/failure_classification.md`
- `docs/backend_baseline_history_triage/follow_up_order.md`
Owning Layer: RV64 callee-saved GPR preservation and object-route live-value
consumption across calls
Queue Order: 66
Proof Surface: current baseline rows 183, 184, and 219 from
`log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`.

## Goal

Repair RV64 callee-saved GPR preservation and object-route live-value
consumption for values that remain live across calls.

## Why This Exists

Step 2 assigned `backend_obj_runtime_rv64_callee_saved_gpr_live_across_call`
to an RV64 prepared GPR/callee-saved runtime owner. It is a singleton row and
therefore follows broader producer/publication families.

The retired static-storage route
`ideas/closed/663_prepared_object_data_static_storage_runtime.md` added rows
183 and 184 to this owner. Their prepared object-data, static layout,
initializer payload, symbols, relocations, and linked data addresses are
coherent; the first observed mismatch is RV64 object-route `main` copying
`a0` into `t0`, then overwriting `t0` from stale `s2` before storing live
values after calls.

## In Scope

- Refresh focused prepared, RV64 assembly, object, disassembly, and runtime
  evidence for the callee-saved GPR row and the routed object-route live-value
  rows.
- Identify whether the first owner is live-range publication, callee-saved
  slot placement, save/restore emission, call clobber modeling, or runtime
  consumption.
- Repair one general callee-saved GPR preservation rule with positive and
  fail-closed evidence.

## Out Of Scope

- Byval call-boundary payloads, pointer-local updates, object-data static
  storage publication/layout/initializer/relocation repair, packed local member
  offsets, destination publication, AArch64, RISC-V object emission, CLI, or
  LLVM torture work.
- Test expectation rewrites, unsupported-marker changes, allowlists, timeout
  changes, runtime policy changes, or baseline acceptance changes.

## Acceptance Criteria

- Focused evidence names the first callee-saved/live-value owner for rows 183,
  184, and 219, or records a justified split.
- The selected repair preserves live values across calls through explicit
  prepared/RV64 facts, save/restore locations, and clobber modeling.
- The focused runtime rows pass or fail closed with precise diagnostics.
- Backend regression proof shows no new backend failures in the supervisor's
  chosen subset.

## Closure Note

Closed after the active runbook completed the focused callee-saved/live-value
route. Step 3 repaired RV64 object-route live-value ordering for rows 183, 184,
and 219, and the focused guard improved from 4/7 before to 7/7 after for the
selected family. Step 4 rechecked rows 87, 88, 100, 101, 183, 184, and 219 and
passed 7/7 with no remaining callee-saved/live-value packet identified.

Close-time regression guard reused canonical same-scope logs
`test_before.log` and `test_after.log` for the seven-test focused family. The
strict monotonic guard reported 7/7 before and 7/7 after with no new failures
but failed only because the lifecycle-close delta had no additional pass-count
increase; rerunning the same comparison with
`--allow-non-decreasing-passed` passed. Remaining backend smoke failures rows
92, 103, 109, 150, 154, 172, 256, 284, and 322 are outside this idea's scope.

## Reviewer Reject Signals

- Reject named-case fixes for the callee-saved GPR row, the routed
  static-local rows, or fixed register identities.
- Reject assuming a register is preserved because final assembly happens to
  work in one test without prepared/RV64 ownership facts.
- Reject rerouting rows 183 or 184 back to static-storage object-data repair
  unless refreshed evidence shows the prepared object-data, static section,
  initializer payload, symbols, relocations, or linked data addresses are the
  first mismatch.
- Reject expectation rewrites, unsupported-marker downgrades, allowlist edits,
  helper renames, or classification-only edits claimed as progress.
- Reject broad rewrites of call lowering, register allocation, or object
  runtime outside the callee-saved GPR preservation owner.
- Reject leaving the same live-across-call corruption behind a new diagnostic
  name.
