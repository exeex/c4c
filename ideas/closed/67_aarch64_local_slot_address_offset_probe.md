# AArch64 Local-Slot Address Offset Probe

Ownership class: evidence-gathering

## Goal

Prove whether the null local-slot address offset path is dead, live, or an
intentionally disabled gap before any contraction or implementation cleanup is
attempted.

## Why This Exists

Idea 59 classified `local_slot_address_frame_offset` as `needs-more-evidence`.
The path appears adjacent to local aggregate address and frame-address
materialization, but the audit did not prove whether callers can reach the null
implementation. This must be answered with a narrow evidence packet before
deleting code, implementing missing prepared offset coverage, or documenting a
disabled path.

## In Scope

- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
  `local_slot_address_frame_offset`.
- Adjacent `local_aggregate_address_frame_offset` and frame-address
  materialization consumers.
- Local-slot address publication paths.
- Existing tests or new narrow probes that can hit or prove dead the null path.

## Out Of Scope

- Contracting unrelated dispatch-family helpers.
- Removing the null path without caller or runtime proof.
- Mutating shared frame-address authority before proving the exact gap.
- Marking a testcase unsupported to avoid the path.

## Acceptance Criteria

- A narrow caller/runtime proof demonstrates whether
  `local_slot_address_frame_offset` is dead, live, or intentionally disabled.
- The result records a clear next route: delete dead code, implement missing
  prepared offset coverage, or keep the disabled path documented.
- No unrelated contraction or shared authority migration is bundled into the
  evidence packet.
- Focused proof covers local-slot address cases likely to reach the path and
  compares them with adjacent aggregate/frame-address behavior.

## Reviewer Reject Signals

- This probe is bundled with unrelated contraction.
- The null path is removed without caller or runtime proof.
- A testcase is marked unsupported or weakened to avoid the path.
- Shared frame-address authority changes before the exact local-slot gap is
  proven.
- The outcome is claimed as capability progress while only recording
  classification evidence.
