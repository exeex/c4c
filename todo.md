# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 9.2
Current Step Title: Reconcile allocation and strict downstream realization

## Just Finished

- Completed Plan Step 9.2's post-review repair: E4 now owns a bounded
  `FrameActionMaterializationTransaction` that inserts explicit fixed-role
  one-record frame actions after D5 copy resolution and before final product
  closure.
- Final ordering is D5 resolution -> private frame-action draft -> E4
  materialization -> constraint projection -> E1 recomputation -> E2/E3
  non-mutating validation -> final `FrameRealizationPlan` ->
  `TargetRealizabilityKey`, with atomic installation and exact
  `FrameActionFingerprint` lineage.
- Synchronized the root/pipeline/verifier, E1-E3, D5/D4, preparation/layout,
  legacy/core adjacency, and F1 boundary. F1 remains strict apply-only and
  produces no hidden frame record.
- Clarified that post-E3 D5 copy resolution stages only private lineage inside
  E4; distinguished D2 per-call preserve/restore from E4 function-frame
  callee-save nodes, and core semantic stack operations from E4 placement
  actions, with exact non-duplicate coverage.

## Suggested Next

- Execute Plan Step 12's complete documentation structure, link, legacy, and
  semantic proof over this repaired checkpoint, then obtain the required fresh
  independent Step 13 review. Do not enter Step 14 unless that review reports
  zero blockers.

## Watchouts

- The E4 frame-action family is deliberately finite:
  `FrameAdjust`, `FrameBaseSetup`/`FrameBaseRestore`,
  `FrameCalleeSave`/`FrameCalleeRestore`, and `FrameProbe`. A supported target
  action outside that family requires a reviewed schema addition; it cannot be
  hidden in the frame plan or synthesized by F1.
- D5 copy resolution has no separately current projected product. Its mutation
  summary is projected together with E4 materialization, and every final
  product must name the materialized revision and `FrameActionFingerprint`.
- D2 call-site preservation and E4 entry/exit callee saves are disjoint, as are
  core semantic stack save/restore/lifetime nodes and E4 frame establishment;
  neither later family may replace or duplicate the earlier semantics.

## Proof

- Passed the supervisor-selected docs-only command:
  `git diff --check` plus the required frame-action/product-key `rg` coverage
  across all owned owner documents and the negative stale-authority search
  over pipeline/verifier/root/MIR.
- The packet explicitly excluded logs, so no `test_after.log` was created or
  modified for this documentation-only proof.
