# RV64 Callee-Saved GPR Runtime Plan

Status: Active
Source Idea: ideas/open/666_rv64_callee_saved_gpr_runtime.md

## Purpose

Turn the RV64 callee-saved/live-value follow-up into an execution runbook that
refreshes evidence, names the first owner, and repairs only the proven
callee-saved GPR preservation or object-route live-value consumption rule.

## Goal

Repair RV64 callee-saved GPR preservation and object-route live-value
consumption for values that remain live across calls without absorbing byval
payloads, pointer-local updates, static-storage object-data repair, packed
member offsets, generic object emission, CLI dump formatting, or other backend
families.

## Core Rule

Prove the live value, the call boundary, the chosen register or spill owner,
and the consumer point before changing callee-saved preservation, clobber
modeling, save/restore emission, or object-route runtime consumption. Do not
claim progress through expectation churn, unsupported-marker downgrades, final
assembly alone, helper renames, fixed-register shortcuts, or testcase identity.

## Read First

- `ideas/open/666_rv64_callee_saved_gpr_runtime.md`
- `ideas/closed/663_prepared_object_data_static_storage_runtime.md`
- `docs/backend_baseline_history_triage/failure_classification.md`
- `docs/backend_baseline_history_triage/follow_up_order.md`

## Current Targets

Focused rows from the current backend baseline:

- row 183 from `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`
- row 184 from `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`
- row 219 from `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`

Primary ownership candidates:

- live-range publication or route fact selection
- callee-saved slot placement
- save/restore emission
- call clobber modeling
- RV64 object-route live-value consumption

Nearby families to keep outside this route unless focused evidence proves the
same first owner:

- byval call-boundary payloads
- pointer-local lowering
- static-storage object-data publication, layout, initializer, symbol, or
  relocation repair
- packed local member offsets
- AArch64 dispatch
- generic RISC-V object emission
- CLI dump formatting
- LLVM torture owner discovery

## Non-Goals

- Do not repair byval payload preservation, pointer-local lowering,
  static-storage object-data publication/layout/initializer/relocation,
  packed local member offsets, destination publication, AArch64 dispatch,
  prepared CLI exposure, generic RISC-V object emission, or LLVM torture rows.
- Do not absorb generic RISC-V object-emission work unless focused evidence
  proves object emission is the first owner for the live-value rows.
- Do not change expectations, unsupported markers, allowlists, timeout policy,
  runtime policy, baseline accounting, or test classification.
- Do not use final assembly or final object bytes as the only authority when
  prepared/RV64 live-value, call-boundary, clobber, or save/restore facts are
  missing or ambiguous.

## Working Model

The active family owns one original callee-saved GPR object-runtime row and
two routed object-runtime rows whose static-storage evidence is coherent but
whose runtime text consumes a stale `s2` value after calls. The first packet
must refresh all three rows and decide whether they share one
callee-saved/live-value owner or need a justified split.

Implementation may start only after evidence identifies one general boundary:
live-range publication, callee-saved slot placement, save/restore emission,
call clobber modeling, RV64 object-route live-value consumption, or a
fail-closed owner that should be routed separately.

## Execution Rules

- Keep routine evidence, packet state, and proof notes in `todo.md`.
- Refresh rows 183, 184, and 219 before selecting an implementation boundary.
- If one target row belongs first to byval, pointer-local, static-storage
  object-data, packed member offsets, generic object emission, or another
  backend family, stop and record the split in `todo.md` for supervisor
  lifecycle routing.
- For each code-changing step, run build proof plus the supervisor-selected
  focused callee-saved/live-value subset. Broader backend regression belongs
  to the supervisor acceptance pass.
- Preserve fail-closed diagnostics for missing, ambiguous, stale, or mismatched
  live-value, call-boundary, clobber, save/restore, or consumer facts.
- Reject patches whose main effect is named-case matching, expectation edits,
  unsupported-marker downgrades, helper renames, classification-only changes,
  fixed-register shortcuts, or hiding the same live-across-call corruption
  behind a new abstraction name.

## Reviewer Reject Signals

- Reject merging byval, pointer-local, static-storage object-data,
  packed-member, generic object emission, CLI, AArch64, or LLVM torture work
  into this route without proof.
- Reject named-case shortcuts for rows 183, 184, or 219, including fixed
  register-identity shortcuts for `s2` or `t0`.
- Reject treating expectation updates, unsupported-marker downgrades,
  allowlist edits, helper renames, diagnostic relabeling, or
  classification-only edits as progress.
- Reject final assembly or object bytes as the only authority when prepared/RV64
  live-value, call-boundary, clobber, or save/restore facts are missing or
  ambiguous.
- Reject rerouting rows 183 or 184 back to static-storage object-data repair
  unless refreshed evidence contradicts the retired 663 findings.
- Reject leaving the same live-across-call corruption behind a renamed helper
  or diagnostic.

## Ordered Steps

### Step 1: Refresh Callee-Saved And Live-Value Evidence

Goal: Reproduce the current failure boundary for the callee-saved row and the
routed object-route live-value rows.

Primary targets:

- current baseline rows 183, 184, and 219
- prepared/RV64 facts for live values across calls
- RV64 assembly, object emission, disassembly, and runtime evidence for all
  focused rows
- diagnostics, missing snippets, or unsupported fragments that name the first
  failure boundary

Actions:

- Run the focused rows and collect the first failing diagnostic, runtime
  mismatch, missing live-value fact, stale register consumer, clobber issue, or
  save/restore issue for each row.
- Refresh semantic, prepared, RV64 assembly, object, disassembly, and runtime
  evidence where needed to identify the first owner.
- Distinguish live-range publication from callee-saved slot placement,
  save/restore emission, call clobber modeling, and RV64 object-route
  live-value consumption.
- Preserve the retired 663 static-storage finding: rows 183 and 184 are not
  static-storage object-data repair candidates unless new evidence contradicts
  it.
- Record whether the three rows share one owner or require a split.

Completion check:

- `todo.md` records a focused evidence summary naming the first owner or split
  for all target rows, with no implementation changes required for this step.

### Step 2: Select The First Callee-Saved/Live-Value Boundary

Goal: Choose one general callee-saved/live-value boundary for the first
implementation packet.

Primary targets:

- the live-range, callee-saved slot, save/restore, clobber-model, or RV64
  consumer surface proven by Step 1
- focused cases that prove a positive shape and a fail-closed negative shape
  for that boundary

Actions:

- Select exactly one boundary from the Step 1 evidence.
- Define the positive evidence the executor must preserve.
- Define the missing, ambiguous, stale, or mismatched evidence that must remain
  fail-closed.
- If the target rows need separate owners, keep only the first
  callee-saved/live-value owner in this runbook and record the remaining
  owner candidate in `todo.md` for supervisor lifecycle routing.
- If the selected boundary belongs to a different backend family, stop before
  implementation and return a lifecycle-routing recommendation.

Completion check:

- `todo.md` identifies one selected implementation boundary, the rows it owns,
  and the focused proof subset for the first repair packet.

### Step 3: Repair The Selected Callee-Saved/Live-Value Rule

Goal: Implement one general callee-saved preservation, clobber-model, or RV64
object-route live-value consumption rule that fixes or fails closed at the
proven owner.

Primary targets:

- the source files named by Step 1 and Step 2 evidence
- the focused backend callee-saved/live-value cases for the selected
  boundary

Actions:

- Repair live-range publication, callee-saved slot placement, save/restore
  emission, call clobber modeling, or RV64 object-route consumption for the
  proven contract.
- Preserve explicit live-value, call-boundary, clobber, save/restore, and
  runtime-consumer evidence in prepared/RV64 facts, disassembly, runtime proof,
  or fail-closed diagnostics.
- Add or update focused positive and negative tests only when they prove the
  general rule rather than the named baseline row.
- Keep rows outside the selected owner fail-closed with precise diagnostics.

Completion check:

- The selected focused subset passes or fails closed at the proven owner.
- Build proof is fresh.
- No unrelated backend family changes are included.

### Step 4: Broaden Within The Callee-Saved/Live-Value Family

Goal: Prove that the repair did not regress nearby callee-saved/live-value
behavior and decide whether another packet remains.

Primary targets:

- rows 183, 184, and 219
- nearby object-data, object-emission, and call-boundary rows only as boundary
  checks, not implementation scope

Actions:

- Re-run the full focused callee-saved/live-value subset after the selected
  repair.
- Include the retired static-storage rows as regression checks for the selected
  live-value owner.
- Check whether remaining failures belong to the same selected owner, a
  separate live-value owner, or a different initiative.
- Record any remaining owner split in `todo.md` without mutating the source
  idea unless a separate initiative is required.

Completion check:

- `todo.md` records proof for the focused family and a clear next packet,
  lifecycle-routing recommendation, or close recommendation for the supervisor.

### Step 5: Supervisor Acceptance Checkpoint

Goal: Hand back a coherent implementation slice with enough proof for
supervisor review.

Actions:

- Ensure the final executor proof includes build plus the delegated focused
  CTest subset.
- Leave canonical broad regression-log decisions to the supervisor.
- Do not close the source idea unless all acceptance criteria in
  `ideas/open/666_rv64_callee_saved_gpr_runtime.md` are satisfied.

Completion check:

- The active plan has an executor-updated `todo.md` with current proof,
  remaining risks, and a commit-readiness recommendation for the supervisor.
