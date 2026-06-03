Status: Active
Source Idea Path: ideas/open/103_prealloc_synthetic_helper_call_abi_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Synthetic Helper ABI Producers And Consumers

# Current Packet

## Just Finished

Activation created the active runbook and executor scratchpad for Step 1.

## Suggested Next

Execute `plan.md` Step 1: inventory synthetic i128 and f128 helper ABI
producers and consumers, including the f128 comparison helper-result bridge.
Record findings here and keep implementation files unchanged for the inventory
packet.

## Watchouts

- Do not treat synthetic helper calls as source BIR direct calls.
- Keep physical clobber, preservation, carrier marshaling, and register/stack
  movement as prealloc authority.
- Separate i128 helper ABI binding, f128 arithmetic/cast helper ABI binding,
  and f128 comparison result bridging during inventory.

## Proof

Activation is lifecycle-only. Run `git diff --check -- plan.md todo.md` for the
activation proof.
