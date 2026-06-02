Status: Active
Source Idea Path: ideas/open/94_aarch64_calls_f128_carrier_operand_owner.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm F128 Carrier Boundary And Proof Subset

# Current Packet

## Just Finished

Activated plan Step 1: `Confirm F128 Carrier Boundary And Proof Subset`.

## Suggested Next

Execute Step 1 by auditing the current f128 carrier helpers and recording the
owner boundary plus exact focused proof command here before any implementation
edits.

## Watchouts

- Keep this route limited to AArch64 calls-side f128 carrier completion and
  q-register operand rendering.
- Do not move prepared f128 carrier creation, shared transport authority,
  aggregate byval lane transport, immediate scalar publication, ordinary
  scalar FP moves, or call-boundary record ownership into the new owner.
- Preserve diagnostics, q-register selection, prepared carrier contracts,
  record fields, and assembly output.
- Keep idea `95` separate unless a supervisor-approved lifecycle switch
  happens.

## Proof

No validation run for activation-only lifecycle work; no implementation files
were touched.
