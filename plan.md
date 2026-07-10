# Prepared Object Data Static Storage Runtime Plan

Status: Active
Source Idea: ideas/open/663_prepared_object_data_static_storage_runtime.md

## Purpose

Turn the prepared object-data/static-storage follow-up into an execution
runbook that refreshes evidence, names the first owner, and repairs only the
proven static-storage object-data or RV64 consumption rule.

## Goal

Repair prepared object-data and RV64 runtime handling for static local storage
and initialized static local storage without absorbing packed member offsets,
byval payloads, generic object emission, CLI dump formatting, or other backend
families.

## Core Rule

Prove storage identity, initialization state, and the consumer point before
changing prepared publication, static storage layout, relocation, or RV64
object-data consumption. Do not claim progress through expectation churn,
unsupported-marker downgrades, final object bytes alone, helper renames, or
testcase identity.

## Read First

- `ideas/open/663_prepared_object_data_static_storage_runtime.md`
- `docs/backend_baseline_history_triage/failure_classification.md`
- `docs/backend_baseline_history_triage/follow_up_order.md`
- `ideas/open/664_riscv_object_emission_internal_probe.md`
- `ideas/open/667_rv64_packed_local_member_offsets.md`
- `ideas/open/670_byval_frame_slot_object_runtime_binaryinst.md`

## Current Targets

Focused rows from the current backend baseline:

- row 183 from `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`
- row 184 from `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`

Primary ownership candidates:

- prepared object-data publication
- static storage identity or layout
- static initialization payload placement
- relocation or symbol binding for static storage
- RV64 object-data consumption

Nearby families to keep outside this route unless focused evidence proves the
same first owner:

- packed local member offsets
- byval call-boundary payloads
- pointer-local lowering
- callee-saved GPR preservation
- CLI dump formatting
- AArch64 dispatch
- generic RISC-V object emission
- LLVM torture owner discovery

## Non-Goals

- Do not repair packed local member offsets in this route.
- Do not repair byval payload preservation, pointer-local lowering,
  callee-saved GPR preservation, destination publication, AArch64 dispatch,
  prepared CLI exposure, or LLVM torture rows.
- Do not absorb generic RISC-V object-emission work unless focused evidence
  proves object emission is the first owner for the static-storage rows.
- Do not change expectations, unsupported markers, allowlists, timeout policy,
  runtime policy, baseline accounting, or test classification.
- Do not use final assembly or final object bytes as the only authority when
  prepared object-data facts are missing or ambiguous.

## Working Model

The active family owns two current object-runtime rows assigned to
prepared object-data/static-storage. The first packet must refresh both rows
and decide whether they share one owner or need a justified split.

Implementation may start only after evidence identifies one general boundary:
prepared object-data publication, static storage layout, initialization payload
placement, relocation or symbol binding, RV64 object-data consumption, or a
fail-closed owner that should be routed separately.

## Execution Rules

- Keep routine evidence, packet state, and proof notes in `todo.md`.
- Refresh both target rows before selecting an implementation boundary.
- If one target row belongs first to packed member offsets, byval, generic
  object emission, or another backend family, stop and record the split in
  `todo.md` for supervisor lifecycle routing.
- For each code-changing step, run build proof plus the supervisor-selected
  focused object-data/static-storage subset. Broader backend regression belongs
  to the supervisor acceptance pass.
- Preserve fail-closed diagnostics for missing, ambiguous, stale, or mismatched
  object-data/static-storage facts.
- Reject patches whose main effect is named-case matching, expectation edits,
  unsupported-marker downgrades, helper renames, classification-only changes,
  or hiding the same static-storage payload or relocation failure behind a new
  abstraction name.

## Reviewer Reject Signals

- Reject merging packed member offsets or generic object emission into this
  static-storage route without proof.
- Reject named-case shortcuts for static local or initialized static local
  storage rows.
- Reject treating expectation updates, unsupported-marker downgrades,
  allowlist edits, helper renames, diagnostic relabeling, or
  classification-only edits as progress.
- Reject final assembly or object bytes as the only authority when prepared
  object-data facts are missing or ambiguous.
- Reject leaving the same static storage payload, symbol, relocation, or RV64
  object-data consumption failure behind a renamed helper or diagnostic.

## Ordered Steps

### Step 1: Refresh Static-Storage Evidence

Goal: Reproduce the current failure boundary for both static local storage
rows.

Primary targets:

- the two current baseline rows 183 and 184
- prepared object-data output for static local storage
- RV64 assembly, object emission, disassembly, and runtime evidence for the
  focused rows
- diagnostics, missing snippets, or unsupported fragments that name the first
  failure boundary

Actions:

- Run the focused static-storage rows and collect the first failing diagnostic,
  runtime mismatch, missing object-data fact, relocation issue, or stale
  consumer evidence for each row.
- Refresh semantic, prepared, RV64 assembly, object, disassembly, and runtime
  evidence where needed to identify the first owner.
- Distinguish prepared object-data publication from static layout,
  initialization payload placement, relocation or symbol binding, and RV64
  object-data consumption.
- Record whether the two rows share one owner or require a split.

Completion check:

- `todo.md` records a focused evidence summary naming the first owner or split
  for both target rows, with no implementation changes required for this step.

### Step 2: Select The First Static-Storage Boundary

Goal: Choose one general object-data/static-storage boundary for the first
implementation packet.

Primary targets:

- the prepared, layout, relocation, object-emission, or RV64 consumer surface
  proven by Step 1
- focused cases that prove a positive shape and a fail-closed negative shape
  for that boundary

Actions:

- Select exactly one boundary from the Step 1 evidence.
- Define the positive evidence the executor must preserve.
- Define the missing, ambiguous, stale, or mismatched evidence that must remain
  fail-closed.
- If the target rows need separate owners, keep only the first
  object-data/static-storage owner in this runbook and record the remaining
  owner candidate in `todo.md` for supervisor lifecycle routing.
- If the selected boundary belongs to a different backend family, stop before
  implementation and return a lifecycle-routing recommendation.

Completion check:

- `todo.md` identifies one selected implementation boundary, the rows it owns,
  and the focused proof subset for the first repair packet.

### Step 3: Repair The Selected Object-Data Rule

Goal: Implement one general static-storage object-data, initialization,
relocation, or RV64 consumption rule that fixes or fails closed at the proven
owner.

Primary targets:

- the source files named by Step 1 and Step 2 evidence
- the focused backend object-data/static-storage cases for the selected
  boundary

Actions:

- Repair publication, layout, initialization payload placement, relocation, or
  RV64 consumption for the proven static-storage contract.
- Preserve explicit storage identity and initialization evidence in prepared
  facts, object data, disassembly, runtime proof, or fail-closed diagnostics.
- Add or update focused positive and negative tests only when they prove the
  general rule rather than the named baseline row.
- Keep rows outside the selected owner fail-closed with precise diagnostics.

Completion check:

- The selected focused subset passes or fails closed at the proven owner.
- Build proof is fresh.
- No unrelated backend family changes are included.

### Step 4: Broaden Within The Static-Storage Family

Goal: Prove that the repair did not regress nearby static-storage behavior and
decide whether another object-data packet remains.

Primary targets:

- both current static-storage rows
- nearby object-data and object-emission rows only as boundary checks, not
  implementation scope

Actions:

- Re-run the full focused object-data/static-storage subset after the selected
  repair.
- Check whether remaining failures belong to the same selected owner, a
  separate static-storage owner, or a different initiative.
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
  `ideas/open/663_prepared_object_data_static_storage_runtime.md` are
  satisfied.

Completion check:

- The active plan has an executor-updated `todo.md` with current proof,
  remaining risks, and a commit-readiness recommendation for the supervisor.
