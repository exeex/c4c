# Current Packet

Status: Active
Source Idea Path: ideas/open/757_lir_computed_goto_address_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Publish computed-goto address authority
你該做code review了

## Just Finished

- Paused 734 after accepted Step 7.23; its next computed-goto receiver requires
  this separate typed-address producer handoff.

## Suggested Next

- Execute Step 1 only: publish and verify `LirIndirectBrOp` address value
  identity.

## Watchouts

- Do not change Raw-BIR receipt or computed-goto successor authority, and do
  not recover an address from `addr`, labels, or printer output.

## Proof

- 734 Step 7.23 accepted: fresh build and matching backend guard 5/5; its
  rejected full baseline candidate is not a green full-suite result.
- Executor: run a fresh build and focused positive/negative producer proof.
- Supervisor: select and record broader/full acceptance separately.
