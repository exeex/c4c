# Shared Prepared Call-Argument Source Selection

## Goal

Introduce a shared prepared call-argument source-selection fact for selected
`BeforeCall` argument moves so target-local call emission can consume a single
owned source choice instead of rederiving caller-side argument sources.

## Why This Exists

The AArch64 calls emission consolidation runbook is blocked because
`lower_before_call_move` still makes target-local source choices for selected
call-argument moves. Existing prepared facts expose argument identity,
destination ABI binding, preservation, memory-return, address materialization,
and byval-adjacent data, but no shared fact says that a selected `BeforeCall`
move should source from prior preservation, local-frame address
materialization, frame-slot or address sources, or byval register-lane
materialization.

That source choice is planning authority, not target-local emission. The
shared call-plan layer should prepare it once and let target emitters lower the
selected source to machine nodes.

## In Scope

- Define a prepared call-argument source-selection representation for selected
  `BeforeCall` argument moves.
- Populate the selection from existing shared prepared call-plan inputs,
  including prior preservation, prepared address materialization, frame-slot or
  address sources, and byval register-lane materialization.
- Expose lookup or direct attachment APIs that let target-local emitters read
  the selected source without rescanning BIR or redoing caller-side planning.
- Update at least the AArch64 caller-side call emission path to consume the
  shared selection as proof that the fact is usable.
- Preserve existing behavior for supported call lowering cases.

## Out Of Scope

- Consolidating or deleting AArch64 `calls*.cpp` files as part of this
  prerequisite.
- Moving target-specific machine-node emission details into the shared call
  planner.
- Broad ABI, dispatch, ALU, memory, comparison, variadic, or prologue cleanup.
- Reclassifying unsupported cases or weakening tests to make the new fact look
  complete.

## Acceptance Criteria

- A shared prepared fact identifies the selected source for relevant
  `BeforeCall` argument moves.
- AArch64 caller-side call emission consumes that fact for the formerly local
  source-selection cases instead of owning the choice in `lower_before_call_move`.
- Existing focused call tests and a fresh build pass after the implementation
  slice.
- The AArch64 calls consolidation idea can be reactivated with its prerequisite
  satisfied, without inventing another target-local source-selection helper.

## Reviewer Reject Signals

- A patch adds AArch64-specific source-selection shortcuts under the shared
  layer instead of modeling target-independent prepared source choices.
- A patch merely renames or wraps the existing AArch64-local
  `lower_before_call_move` decision tree while preserving the same local
  planning authority.
- A patch handles only one named testcase or one named function shape while the
  nearby prior-preservation, local-frame address, frame-slot/address, or byval
  lane cases remain unmodeled or unexamined.
- A patch weakens, deletes, or reclassifies call-lowering tests without explicit
  user approval.
- A patch claims prerequisite completion without showing that AArch64 consumes
  the shared prepared source-selection fact for selected `BeforeCall` argument
  moves.
