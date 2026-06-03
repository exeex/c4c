Status: Active
Source Idea Path: ideas/open/104_bir_prealloc_pointer_carrier_provenance_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Pointer Carrier Seeds And Propagation

# Current Packet

## Just Finished

Activation created the active runbook and executor scratchpad for Step 1.

## Suggested Next

Execute `plan.md` Step 1: inventory pointer-carrier seeds and propagation in
`src/backend/prealloc/regalloc/pointer_carriers.cpp`, including local-slot
pointer storage, pointer-symbol seeding, and pointer plus/minus carrier
inference. Record findings here and keep implementation files unchanged for the
inventory packet.

## Watchouts

- Do not treat load/store instruction shape as target-neutral provenance
  authority without a structured fact.
- Keep register allocation, storage homes, spill decisions, and target
  placement in prealloc.
- Separate transient regalloc carrier metadata from durable BIR/prepared
  pointer provenance.

## Proof

Activation is lifecycle-only. Run `git diff --check -- plan.md todo.md` for the
activation proof.
