# Byval Frame-Slot Object Runtime BinaryInst Runbook

Status: Active
Source Idea: ideas/open/670_byval_frame_slot_object_runtime_binaryinst.md

## Purpose

Classify and repair the object-runtime `BinaryInst` unsupported-fragment
failure that remains for RV64 frame-slot pointer-argument payload preservation
after the focused byval prepared call-boundary route.

Goal: prove the first owner for
`backend_obj_runtime_rv64_frame_slot_pointer_arg_preserves_payload`, then
repair only the general instruction or object-runtime contract that evidence
selects.

## Core Rule

Treat the completed byval route and runtime rows as regression surfaces. Do
not reopen prepared byval call-boundary behavior unless fresh focused evidence
shows those passing rows regressed.

## Read First

- `ideas/open/670_byval_frame_slot_object_runtime_binaryinst.md`
- `ideas/closed/659_rv64_byval_prepared_call_boundary.md`
- `ideas/closed/664_riscv_object_emission_internal_probe.md`
- `docs/backend_baseline_history_triage/failure_classification.md`
- `docs/backend_baseline_history_triage/follow_up_order.md`

## Current Target

- `backend_obj_runtime_rv64_frame_slot_pointer_arg_preserves_payload`

Nearby regression surfaces:

- byval prepared call-boundary route rows from idea 659
- byval runtime rows from idea 659
- focused RISC-V object-emission rows from idea 664, if evidence shows the
  first owner is object emission rather than object-runtime instruction support

## Non-Goals

- Do not work on prepared-BIR dump snippet review for byval aggregate or
  preserved pointer-argument dump rows.
- Do not rework RV64 byval runtime or codegen-route behavior that already
  passed under idea 659.
- Do not absorb static object-data storage, packed local member offsets,
  destination publication, callee-saved GPR, pointer-local, AArch64, CLI, LLVM
  torture, or generic RISC-V object-emission work without focused first-owner
  evidence from the target row.
- Do not change unsupported markers, allowlists, timeout policy, runtime
  policy, or baseline acceptance.
- Do not claim expectation, diagnostic, helper-name, or classification-only
  edits as object-runtime capability progress.

## Working Model

The selected row currently belongs to the object-runtime surface because the
byval route/runtime rows were already proven elsewhere, while this row reported
`unsupported_instruction_fragment` for `instruction_kind=BinaryInst`.
Execution must start by refreshing that boundary and deciding whether the
owner is object-runtime `BinaryInst` support, RV64 object emission,
relocation/object writer behavior, or a missing prepared fact consumed by
object runtime.

## Execution Rules

- Refresh the focused object-runtime row before selecting a repair owner.
- Keep final object bytes as evidence only after the instruction/object-runtime
  contract is named.
- If the owner is object-runtime `BinaryInst`, repair a general instruction
  handling rule rather than matching the testcase.
- If the owner is object emission or relocation/writer behavior, prove why the
  object-runtime contract is already satisfied before changing that layer.
- If the owner is a missing prepared fact, stop before reopening byval prepared
  work unless the focused evidence directly proves the missing fact.
- Preserve precise fail-closed diagnostics for unsupported or ambiguous
  instruction fragments.
- Use focused proof first, then add nearby byval and object-emission regression
  proof before close.

## Step 1: Refresh Object-Runtime BinaryInst Boundary

Goal: establish the current first observable boundary for the object-runtime
row.

Primary target:

- `backend_obj_runtime_rv64_frame_slot_pointer_arg_preserves_payload`

Actions:

- Run focused evidence for the target row.
- Capture the current diagnostic, instruction kind, fragment location, and any
  prepared facts consumed by object runtime.
- Compare the result against the closed byval prepared call-boundary route and
  current object-emission closure evidence.
- Record whether the first owner appears to be object-runtime `BinaryInst`,
  RV64 object emission, relocation/object writer behavior, or missing prepared
  fact publication.

Completion check:

- `todo.md` records the focused command, result, first diagnostic boundary, and
  a concrete owner classification or a precise reason ownership remains
  blocked.

## Step 2: Select Repair Owner And Patch Narrowly

Goal: repair exactly the proven general owner for the target row.

Actions:

- If the owner is object-runtime `BinaryInst`, implement the general supported
  instruction handling needed for the observed fragment.
- If the owner is RV64 object emission, relocation, or writer behavior, repair
  only the proven object-layer rule and keep object-runtime diagnostics
  precise.
- If the owner is missing prepared publication, hand back the evidence instead
  of silently expanding this idea into prepared byval call-boundary work.
- Keep the existing byval route/runtime rows unchanged except for regression
  proof.
- Avoid testcase-name, final-byte, fixed-register, or source-shape shortcuts.

Completion check:

- The focused object-runtime row passes or fails closed with a more precise
  proven owner, and the diff shows a general rule rather than named-case
  handling.

## Step 3: Prove Byval Object-Runtime Regression Safety

Goal: prove the repair did not regress nearby byval or object-emission
surfaces.

Actions:

- Rerun the focused object-runtime row after the repair.
- Run nearby byval route/runtime rows from idea 659 as regression surfaces.
- If the repair touched object emission, include the focused object-emission
  rows from idea 664.
- Record exact commands and results in `todo.md`.

Completion check:

- Focused target proof and nearby regression proof are green, or any remaining
  failure is fail-closed with an owner outside idea 670's scope.

## Step 4: Close Readiness

Goal: prepare the lifecycle close decision for idea 670.

Actions:

- Confirm the source idea acceptance criteria are satisfied.
- Confirm no forbidden scope was changed.
- Ensure canonical regression logs cover the supervisor-selected close scope.
- Hand off to the plan owner for close only after focused proof and regression
  proof are current.

Completion check:

- `todo.md` contains close-ready proof notes, and the active runbook can be
  evaluated against `ideas/open/670_byval_frame_slot_object_runtime_binaryinst.md`.
