# Current Packet

Status: Active
Source Idea Path: ideas/open/81_aarch64_dispatch_edge_copy_owner_contraction.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Build the edge-copy helper classification table

## Just Finished

Lifecycle activation initialized this packet from
`ideas/open/81_aarch64_dispatch_edge_copy_owner_contraction.md`.

## Suggested Next

Execute Step 1 from `plan.md`: inventory the remaining helpers in
`dispatch_edge_copies.*`; classify producer-context lookup, source fact
validation, load-local emission, typed stack-source emission, block-entry move
filtering, select parallel-copy lowering, and dispatch orchestration; then
record the first bounded contraction packet plus proof command here before
moving code.

## Watchouts

- Do not re-derive prepared edge-copy or typed stack-source facts locally.
- Do not mix dispatch-publication relocation into this plan.
- Do not change edge-copy ordering behavior.
- Do not weaken edge-copy or block-entry publication expectations.

## Proof

No code proof required for lifecycle-only activation.
