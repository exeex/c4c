# Current Packet

Status: Active
Source Idea Path: ideas/open/747_lir_direct_branch_successor_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Publish and verify one direct LirBr successor ID

## Just Finished

- Plan Step 1 complete: every direct `LirBr` producer now carries a
  current-function structural `LirBlockId` successor; verifier and focused
  native malformed-neighbour coverage fail closed without label recovery.

## Suggested Next

- Lifecycle owner should conclude this producer blocker and reactivate idea
  734 at its Step 6.3 direct-jump receiver return point.

## Watchouts

- `target_label` remains display parity only. Conditional, switch, indirect,
  phi, Raw-BIR, and importer work remain outside this packet.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$' > test_after.log` passed; log: `test_after.log`.
