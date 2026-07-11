# BIR To Prealloc Rebuild Layout Review

Status: Open
Type: Phoenix rebuild stage
After: `ideas/open/679_bir_to_prealloc_rebuild_extraction.md`
Parent: `ideas/open/679_bir_to_prealloc_rebuild_extraction.md`
Stage: 2 of 4
Read This First: `.codex/skills/phoenix-rebuild/SKILL.md`
Handoff Directory: `docs/bir_to_prealloc_rebuild/layout_review/`
Related:
- `docs/bir_to_prealloc_rebuild/extraction/`
- `ideas/open/647_ordered_or_exclusive_stack_destination_fan_in_authority.md`
- `ideas/open/655_stack_destination_fan_in_authority_decomposition.md`
- `ideas/open/678_lir_to_bir_adapter_boundary_umbrella.md`

## Intent

Review the stage-1 extraction and design the replacement `BIR -> prepared`
layout. Decide what should survive, what should become compatibility, what
should be deleted, and which seams should own authority/provenance before MIR
consumers read prepared facts.

## Stage In Sequence

Stage 2 of 4. This stage consumes extraction artifacts only; it does not draft
replacement file contents and does not implement the rewrite.

## Produces

- `docs/bir_to_prealloc_rebuild/layout_review/replacement_layout.md`.
- `docs/bir_to_prealloc_rebuild/layout_review/stage2_to_stage3_handoff.md`.
- A decision on whether stage-1 extraction needs correction before drafting.
- The exact replacement `.cpp.md` / `.hpp.md` artifact layout that stage 3 must
  produce.

The layout review must name intended replacement interface families, including:

- semantic BIR input view
- prepared analysis pipeline
- authority and provenance publication
- storage/home/value-location model
- call/ABI and helper planning
- target/MIR consumer views
- compatibility adapters for legacy prepared dumps

## Does Not Yet Own

- Writing replacement `.cpp.md` / `.hpp.md` drafts.
- Real implementation files.
- Deleting legacy files.
- Repairing failing tests.
- Changing baseline policy beyond documenting the allowed rebuild-stage
  exclusion contract.

## Unlocks

Unlocks stage 3 by providing a concrete replacement artifact map and intake
handoff for draft authors.

## Scope Notes

The review must explicitly judge whether the replacement layout addresses
historical rebuild pressure from:

- stack destination fan-in authority gaps in ideas 647 and 655;
- broad prepared publication growth;
- route-index facade coupling;
- prepared module aggregate sprawl;
- MIR consumer dependence on internal prepared state;
- RV64 GCC torture residual pressure without testcase-shaped fixes.

The stage-2 handoff must say which stage-1 artifacts are trustworthy, which
need repair, which replacement drafts are mandatory, and what route constraints
must be preserved.

## Boundaries

- Do not implement.
- Do not draft replacement source files before the layout is accepted.
- Do not mix LIR import cleanup from idea 678 into this BIR->prealloc rewrite.
- Do not make target-specific RV64/AArch64/x86 facts canonical prepared input.
- If `.plan_review_state.json` uses `test_baseline_exclude_regex` during this
  stage, the review must record the exact regex and why each excluded test
  family belongs to the active rebuild scope.

## Completion Signal

This idea can close only when:

- `replacement_layout.md` reconstructs the current subsystem shape and
  proposes a concrete replacement layout;
- it states whether stage-1 extraction needs correction before stage 3;
- it explains how the layout addresses the motivating failure families;
- `stage2_to_stage3_handoff.md` lists mandatory draft files, trusted inputs,
  required corrections, and route constraints;
- no implementation, test expectation, unsupported marker, allowlist, runtime
  behavior, tracked build artifact, `plan.md`, or `todo.md` change is claimed
  as layout progress.

## Reviewer Reject Signals

- Reject layout documents that only restate the old files with new names.
- Reject a design that keeps one giant prepared aggregate as the only
  interface.
- Reject designs that let MIR consumers query arbitrary prepared internals.
- Reject designs that do not address authority/provenance publication.
- Reject direct implementation or testcase-shaped repair in this stage.
- Reject stale or overly broad baseline exclusions.
