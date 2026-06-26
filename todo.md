Status: Active
Source Idea Path: ideas/open/390_rv64_va_list_value_publication_copy_runtime_abort.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture The `va_list` Value Publication Boundary

# Current Packet

## Just Finished

Lifecycle activation created the active runbook for Step 1 of idea 390.

## Suggested Next

Execute Step 1: capture the prepared/BIR/object facts for the initialized
`va_list` value and the later `dummy(statep->ap)` / `dummy(state.ap)`
call-argument uses in `va-arg-13.c`.

## Watchouts

- Keep idea 389 `va_start` destination-address materialization closed.
- Keep idea 388 `va_end`, idea 386 frame-slot-address GPR call arguments, and idea 387 sret out of scope.
- Do not hard-code `va-arg-13.c`, `test`, `dummy`, registers, stack offsets, or the abort branch.
- Do not broaden this packet into a full `va_arg`, aggregate, or call ABI rewrite.

## Proof

Lifecycle-only activation. No build or test proof required.
