# 198 Phase D versus Phase E lifecycle naming cleanup

## Goal

Clarify lifecycle naming so ideas 182-189 are treated as Phase D follow-up
selected-consumer migrations despite their `phase_e_*` filenames, while draft
155 remains the future true Phase E `PreparedBirModule` retirement plan.

## Why This Exists

The pre-Phase-E readiness audit found a durable naming hazard: closed ideas
182-189 carry Phase E-like filenames, but their actual scope is Phase D
follow-up route-view migration. If future agents read the filenames as true
Phase E retirement completion, they may open draft 155 too early or claim
prepared API contraction from selected-consumer evidence.

## In Scope

- Produce a lifecycle/naming note that classifies ideas 182-189 as Phase D
  follow-ups.
- Define how future plans should reference these ideas without treating them as
  the true Phase E retirement plan.
- State that draft 155 remains draft-only until prerequisite maps exist.
- Identify where references should be clarified if future documentation cites
  ideas 182-189 as Phase E.
- Preserve historical filenames unless a separate, explicit lifecycle decision
  approves renaming.

## Out Of Scope

- Renaming closed idea files for cosmetic cleanup.
- Rewriting implementation history or changing closed idea contents solely to
  match new labels.
- Opening or executing draft 155.
- Claiming that selected-consumer route-view migrations satisfy
  `PreparedBirModule` retirement acceptance criteria.
- Combining naming cleanup with implementation or API contraction work.

## Acceptance Criteria

- A durable naming note states that ideas 182-189 are Phase D follow-up slices
  and draft 155 is the future true Phase E retirement analysis.
- Future references to ideas 182-189 have clear language for "Phase D
  follow-up" versus "Phase E-like filename".
- The note names the prerequisites that must exist before draft 155 can be
  activated or rewritten into an active plan.
- No closed idea files are renamed as part of routine cleanup.
- The cleanup reduces lifecycle ambiguity without changing implementation
  contracts or claiming retirement progress.

## Reviewer Reject Signals

- Renaming closed files cosmetically in a way that obscures review history.
- Opening draft 155 before the prerequisite ownership/readiness maps exist.
- Treating ideas 182-189 as true Phase E completion because of their filenames.
- Claiming progress through label changes while leaving retirement blockers
  unresolved.
- Editing implementation code or prepared API contracts under a naming-cleanup
  scope.
- Weakening source-idea or test expectations to make lifecycle status appear
  more complete.

## Completion Note

Closed after producing
`docs/bir_prealloc_fusion/phase_d_phase_e_lifecycle_naming_cleanup.md`.
The note classifies ideas 182-189 as Phase D follow-up slices despite their
`phase_e_*` filenames, preserves historical closed filenames, and keeps draft
155 as the future true Phase E `PreparedBirModule` retirement analysis.
