# Current Packet

Status: Active
Source Idea Path: ideas/open/757_lir_computed_goto_address_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Publish computed-goto address authority

## Just Finished

- Closed prerequisite 758: commit `c8a205218` preserves eligible local and
  parameter rvalue `LirValueId` authority through the rvalue/operand route.

## Suggested Next

- Execute Step 1 only: publish and verify `LirIndirectBrOp` address value
  identity from the preserved rvalue identity.

## Watchouts

- Do not change Raw-BIR receipt or computed-goto successor authority, and do
  not recover an address from `addr`, labels, or printer output. Missing,
  invalid, foreign, and non-pointer authority must fail closed.

## Proof

- 758 accepted: fresh build and focused frontend proof passed; matching
  `^backend_` pre/post baselines passed 5/5, and the full-suite hook candidate
  was accepted at 3034/3034 pre/post.
- Executor: run a fresh build and the focused positive/negative producer proof.
- Supervisor: select and record broader/full acceptance separately.
