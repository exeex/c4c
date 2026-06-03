Status: Active
Source Idea Path: ideas/open/102_aapcs64_va_arg_payload_shape_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Existing Va Arg Shape Authority

# Current Packet

## Just Finished

Activation created the active runbook and executor scratchpad for Step 1.

## Suggested Next

Execute `plan.md` Step 1: inventory scalar, aggregate, and HFA AAPCS64
`va_arg` payload authority across BIR placeholder production and prealloc
variadic entry planning. Record findings here and keep implementation files
unchanged for the inventory packet.

## Watchouts

- Do not add or endorse slot-name parsing as payload-shape authority.
- Keep fixed-call HFA pressure and unrelated AArch64 ABI behavior out of scope.
- Treat scalar payload ABI, aggregate destination-home behavior, and HFA lane
  shape as separate facts during inventory.

## Proof

Activation is lifecycle-only. Run `git diff --check -- plan.md todo.md` for the
activation proof.
