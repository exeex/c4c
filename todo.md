# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 9.2
Current Step Title: Reconcile allocation and strict downstream realization

## Just Finished

- Fresh Plan Step 13 review at
  `review/731_final_frame_action_architecture_review.md` rejected Step 14 with
  one bounded Step 9.2/Step 10 adjacency blocker.
- The explicit E4 frame-action producer/schema, ownership ledgers, exact-current
  E1/E2/E3/frame/target route, verifier intervals, legacy inventory, D2/C9
  authority separation, strict F1 boundary, and docs-only scope remain resolved
  checkpoints. Idea 731 remains open and unchanged.

## Suggested Next

- Execute one bounded Plan Step 9.2 packet reopening only Step 10 adjacency:
  repair `src/backend/bir/regalloc/constraints/README.md` as the sole projection
  owner, then recheck `passes/out_of_ssa/README.md`, `allocated/README.md`,
  `pseudo/README.md`, `verify/README.md`, and the root for the same route.
- The repaired route must say private D5 contributes complete mutation lineage
  without publishing a projection; E4 invokes projection once after frame-action
  materialization; the final key binds the materialized stamp,
  `CopyResolutionFingerprint`, `FrameActionFingerprint`, and both mutation/
  replacement/tombstone summaries.
- Repeat Step 12 after the focused repair, then obtain a new independent Step 13
  review. Step 14 remains forbidden until that exact review reports zero
  blockers.

## Watchouts

- The blocker is in the sole projection owner: it still requires a separate
  pre-E4 D5 projection and omits E4 frame-action occurrence lineage and
  `FrameActionFingerprint`. Do not alter the already-selected E4 route or add a
  second projection authority.
- Preserve the final atomic order: private D5 resolution -> frame-action draft
  -> E4 materialization -> sole constraint projection -> E1 -> E2 -> E3 ->
  frame plan -> target realizability. No predecessor projection may be relabeled
  current or published from private D5 staging.
- Preserve all completed checkpoints, especially the finite frame-action schema,
  strict no-late-repair/F1 apply-only boundary, distinct verifier intervals,
  exact legacy ledger, and D2/C9 ownership.

## Proof

- Lifecycle reset only. The focused executor must record fresh docs-only proof
  for the sole-owner key and invocation route. Step 12 must then repeat the
  complete structure, links, legacy inventory, and semantic/negative checks
  before the new independent Step 13 review.
