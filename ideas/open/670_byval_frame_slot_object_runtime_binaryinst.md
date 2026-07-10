# Byval Frame-Slot Object Runtime BinaryInst Support

Status: Open
Type: Implementation
Parent: `ideas/open/658_backend_baseline_history_umbrella_triage.md`
Related:
- `ideas/closed/659_rv64_byval_prepared_call_boundary.md`
- `ideas/open/664_riscv_object_emission_internal_probe.md`
- `docs/backend_baseline_history_triage/failure_classification.md`
- `docs/backend_baseline_history_triage/follow_up_order.md`
Owning Layer: RV64 object-runtime instruction support for byval frame-slot
payload preservation
Queue Order: 70
Proof Surface: residual row from the closed RV64 byval prepared call-boundary
route:
- `backend_obj_runtime_rv64_frame_slot_pointer_arg_preserves_payload`

## Goal

Classify and repair the object-runtime `BinaryInst` unsupported-fragment
failure that remains for RV64 frame-slot pointer-argument payload preservation
after the byval runtime and codegen-route rows pass.

## Why This Exists

The focused byval proof for idea 659 left the object-runtime row failing with
`unsupported_instruction_fragment` for `instruction_kind=BinaryInst`, while
the selected RV64 byval runtime and codegen-route rows passed. That evidence
points to object-runtime instruction support or object-emission consumption as
a separate owner from the completed prepared call-boundary repair.

## In Scope

- Refresh focused object-runtime evidence for
  `backend_obj_runtime_rv64_frame_slot_pointer_arg_preserves_payload`.
- Identify whether the first owner is object-runtime `BinaryInst` support,
  RV64 object emission, relocation/object writer behavior, or a missing
  prepared fact consumed by object runtime.
- Repair one general object-runtime or object-emission rule only after the
  unsupported fragment owner is proven.
- Keep the closed byval route/runtime rows as nearby regression proof.

## Out Of Scope

- Prepared-BIR dump snippet review for the byval aggregate and preserved
  pointer-argument dump rows.
- Reworking RV64 byval runtime or codegen-route behavior that already passed
  under idea 659.
- Static object-data storage, packed local member offsets, destination
  publication, callee-saved GPR, pointer-local, AArch64, CLI, or LLVM torture
  work unless focused evidence proves this row shares their first owner.
- Expectation rewrites, unsupported-marker changes, allowlist edits, timeout
  changes, runtime policy changes, or baseline acceptance changes.

## Acceptance Criteria

- Focused evidence names the first owner for the `BinaryInst`
  unsupported-fragment row.
- The selected repair handles the proven instruction/object-runtime contract
  generally, without matching this testcase by name.
- The focused object-runtime row passes or fails closed with precise
  diagnostics at the proven owner.
- Backend regression proof shows no new backend failures in the supervisor's
  chosen subset.

## Reviewer Reject Signals

- Reject named-case fixes for
  `backend_obj_runtime_rv64_frame_slot_pointer_arg_preserves_payload`.
- Reject absorbing generic RISC-V object-emission or static object-data work
  without concrete evidence that it is the first owner for this row.
- Reject expectation rewrites, unsupported-marker downgrades, allowlist edits,
  timeout changes, runtime policy changes, helper renames, or classification
  changes claimed as capability progress.
- Reject final object bytes as the only authority when the object-runtime
  instruction contract is not named.
- Reject reopening the completed byval prepared call-boundary route unless a
  fresh focused proof shows the passing route or runtime rows regressed.
