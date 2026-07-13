Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Implement the verified LIR-to-BIR carrier boundary

# Current Packet

## Just Finished

- Lifecycle reset only; no implementation packet has completed under the
  narrowed runbook.

## Suggested Next

- Delegate Step 1 as one bounded executor packet with a backend-enabled build
  and `backend_lir_to_bir_interface` as the only acceptance test.

## Watchouts

- Do not absorb the dirty broad docs/README/review slice into this packet.
- Stop at verified target-independent Raw/Canonical BIR; allocation, target
  budgets, spill/reload, and MIR remain deferred.

## Proof

- Lifecycle-only reset; implementation proof is pending.
