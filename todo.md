# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 9.2
Current Step Title: Reconcile allocation and strict downstream realization

## Just Finished

- Repeated Plan Step 13 review is complete and rejected architecture
  acceptance with two remaining Step 9.2 blockers. The authoritative reset
  input is `review/731_post_repair_full_architecture_review.md`.
- The exact-current D5 product chain, verifier intervals, D2/C9 contracts,
  legacy inventory, strict no-late-repair rule, and docs-only boundary remain
  resolved checkpoints. Idea 731 remains open and unchanged.

## Suggested Next

- Execute Plan Step 9.2 as one bounded documentation packet: choose and
  synchronize one explicit BIR-owned post-E3 producer/schema route for required
  final-frame actions, then reconcile the pipeline and verifier ownership
  ledgers so E4 owns the exact private `FrameRealizationPlan` and F1 is
  apply-only. The packet must cover `pseudo/README.md`,
  `passes/out_of_ssa/README.md`, `allocated/README.md`,
  `pipeline/README.md`, `verify/README.md`, and root/MIR adjacency; touch the
  E1-E3 owners only if the selected exact-current recomputation route changes
  their contracts.
- After the Step 9.2 packet, repeat Step 12's complete documentation proof and
  obtain a new independent Step 13 review. Do not enter Step 14 unless that
  exact post-repair review reports zero blockers.

## Watchouts

- Blocker 1: the current closed pseudo schema has no admitted producer/variant
  for required frame setup/teardown, stack adjustment, callee-save/restore, or
  another final-frame action after E3. Failure-only handling for ordinary
  spill/call frames is not a supported realization route.
- Blocker 2: `pipeline/README.md` and `verify/README.md` still assign frame
  layout/storage authority to MIR/backend or outside BIR despite the root and
  allocated owner assigning the exact private plan to E4.
- If the chosen route mutates after E3, it must fully reproject/recompute the
  exact-current projection, E1, E2, E3, frame, and realizability products and
  commit atomically. Preserve the distinct public allocation-free and private
  assigned verifier intervals, legacy ledger, D2/C9 ownership, and strict F1
  non-repair boundary.

## Proof

- Lifecycle reset only. The Step 9.2 executor must record fresh docs-only proof
  here; Step 12 must then repeat the complete structure, link, legacy, and
  semantic checks before the new independent Step 13 review.
