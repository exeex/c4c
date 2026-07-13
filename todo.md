# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 9.2
Current Step Title: Reconcile allocation and strict downstream realization

## Just Finished

- Completed the reopened Plan Step 9.2 / Step 10 projection-adjacency repair
  from `review/731_final_frame_action_architecture_review.md`.
- The sole C9 `ConstraintProjectionTransaction` owner now states that post-E3
  D5 copy resolution stages its candidate, `CopyResolutionFingerprint`, and
  complete mutation/replacement/tombstone lineage privately inside E4, with no
  standalone projection, revision, product, or consumer boundary.
- After `FrameActionMaterializationTransaction`, E4 invokes that sole authority
  exactly once. The final `ProjectedConstraintKey` binds the materialized
  `PipelineStageStamp`, both copy/frame-action fingerprints, and both complete
  mutation/replacement/tombstone summary families.
- Projection is explicitly first in E4's atomic six-product closure, followed
  by exact-current E1, E2, E3, frame-realization, and target-realizability
  products. Earlier D1/D2/D4/initial-D5/E3 projection rules remain unchanged.

## Suggested Next

- Repeat Plan Step 12's complete 44-document structure/link, exact legacy, and
  semantic/negative proof against this focused repair, then obtain a fresh
  independent Step 13 review. Step 14 remains forbidden until that exact review
  reports zero blockers.

## Watchouts

- Private post-E3 D5 resolution must never gain a separately current
  `ProjectedConstraintSet`; its full lineage is consumed only by E4's one
  post-materialization projection invocation.
- Preserve final order: D5 private staging -> frame-action draft -> frame-action
  materialization -> projection -> E1 `LivenessInterferenceKey` -> E2
  `AssignmentKey` -> E3 `SpillStateKey` -> `FrameRealizationPlan`/key ->
  `TargetRealizabilityKey`, installed atomically.
- Stable IDs, structural equality, preservation records, or predecessor keys
  never establish final-revision freshness. Do not add another binder or
  projection owner.

## Proof

- Passed `git diff --check`.
- Positive proof passed:
  `positive PASS: sole post-materialization projection binds both fingerprints
  and both mutation families; six-product E4 closure ordered and atomic`.
- Focused positive searches covered the sole projection owner, private D5
  staging/no projection, exact post-materialization E4 invocation, materialized
  stamp, both fingerprints, both mutation/replacement/tombstone summaries, all
  six exact-current products, invalidation, rollback, and atomic installation
  across the owner and adjacent contracts.
- Focused negatives found no affirmative separate post-E3 D5 projection or
  publication, final projection before materialization, predecessor relabeling,
  alternate projection authority, F1/MIR repair, acceptance marker,
  testcase-shaped shortcut, expectation downgrade, or test-file change.
- `git status --short` contains only the owned constraints document and
  `todo.md`, plus the three pre-existing untracked review reports.
- Docs-only packet: no build/test subset applies and no regression log was
  created or modified.
