Status: Active
Source Idea Path: ideas/open/214_aarch64_aapcs64_call_return_frame_mir_contract.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inspect Existing Call Return Frame Assumptions

# Current Packet

## Just Finished

Lifecycle activation created the active runbook and initialized this execution
state from `ideas/open/214_aarch64_aapcs64_call_return_frame_mir_contract.md`.

## Suggested Next

Start Step 1 by inspecting existing AArch64 call, return, frame, prologue,
ABI, and ledger assumptions. Record findings here before implementation or
contract edits.

## Watchouts

- Do not start call, return, or prologue instruction selection under this plan.
- Do not treat legacy assembly-text emitter behavior as current backend
  authority.
- Keep `x8`, `x16`, `x17`, `x29`, and `x30` special-register roles explicit.
- Split missing prepared carriers into separate open ideas instead of inventing
  AArch64-local facts.

## Proof

Activation-only lifecycle work. No build required.
