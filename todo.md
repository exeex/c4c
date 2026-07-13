Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Freeze the MIR-ready abstract-BIR boundary

# Current Packet

## Just Finished

- Lifecycle reset only; no implementation packet is accepted by this repair.

## Suggested Next

- Repair the architecture/checkpoint documents to the Step 1 contract, then
  request independent adjacency review.

## Watchouts

- Step 2 remains unauthorized until Step 1 and the ordered architecture are
  accepted. Do not restore deleted MIR documents or make PreparedBir a second
  instruction graph.

## Proof

- Lifecycle-only repair: run `git diff --check`.
