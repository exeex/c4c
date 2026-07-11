# BIR To Prealloc Rebuild Replacement Draft

Status: Open
Type: Phoenix rebuild stage
After: `ideas/open/680_bir_to_prealloc_rebuild_layout_review.md`
Parent: `ideas/open/680_bir_to_prealloc_rebuild_layout_review.md`
Stage: 3 of 4
Read This First: `.codex/skills/phoenix-rebuild/SKILL.md`
Handoff Directory: `docs/bir_to_prealloc_rebuild/replacement_draft/`
Related:
- `docs/bir_to_prealloc_rebuild/layout_review/replacement_layout.md`
- `docs/bir_to_prealloc_rebuild/layout_review/stage2_to_stage3_handoff.md`

## Intent

Draft the replacement `BIR -> prepared/prealloc` subsystem as markdown source
artifacts before implementation. Define interfaces, ownership, dependency
direction, and compatibility boundaries for every replacement component named
by stage 2.

## Stage In Sequence

Stage 3 of 4. This stage produces reviewed replacement drafts only; real source
files wait for stage 4.

## Produces

- One `.cpp.md` for every planned replacement implementation file declared by
  stage 2.
- One `.hpp.md` for each directory-index non-helper replacement header declared
  by stage 2.
- `docs/bir_to_prealloc_rebuild/replacement_draft/index.md`.
- `docs/bir_to_prealloc_rebuild/replacement_draft/draft_review.md`.

Each draft must state:

- owned responsibility;
- owned inputs;
- owned outputs;
- indirect queries allowed;
- forbidden knowledge;
- classification as core logic, dispatch, optional fast path, or compatibility.

## Does Not Yet Own

- Real implementation.
- Dispatcher rewiring.
- Legacy deletion.
- Test expectation changes.
- Baseline acceptance.

## Unlocks

Unlocks stage 4 by producing a coherent, reviewed implementation contract that
executors can convert into source through staged migration.

## Scope Notes

Drafts should be partitioned by responsibility, not legacy line ranges. The
expected responsibility seams include:

- semantic BIR input normalization;
- prepared analysis phase orchestration;
- authority and provenance publication;
- value/home/storage resolution;
- call/ABI/helper planning;
- consumer view construction;
- diagnostics and prepared dump compatibility.

If a draft needs two components to know each other's full internal context, the
seam is not clean enough and stage 2 must be repaired.

## Boundaries

- Follow the exact artifact map from stage 2. Adding or removing planned
  replacement files is a stage-2 contract repair, not silent drift.
- Do not copy old special cases without classification.
- Do not implement or delete legacy code.
- Do not use baseline exclusion as proof of draft quality; exclusion only
  protects active rebuild work from unrelated baseline acceptance pressure.

## Completion Signal

This idea can close only when:

- every stage-2 planned replacement `.cpp` / `.hpp` has a corresponding draft;
- the draft index points at the full artifact set;
- `draft_review.md` judges the set coherent enough for implementation
  conversion;
- every special case is classified;
- every draft names owned inputs, outputs, indirect queries, and forbidden
  knowledge;
- no implementation, test expectation, unsupported marker, allowlist, runtime
  behavior, tracked build artifact, `plan.md`, or `todo.md` change is claimed
  as draft progress.

## Reviewer Reject Signals

- Reject drafts that are annotated copies of legacy source.
- Reject catch-all components that preserve the old ownership confusion.
- Reject interfaces that require consumers to inspect arbitrary prepared
  internals.
- Reject unclassified fast paths or special cases.
- Reject implementation edits, expectation rewrites, unsupported-marker
  changes, allowlist filtering, or baseline acceptance under this stage.
