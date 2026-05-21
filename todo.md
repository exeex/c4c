Status: Active
Source Idea Path: ideas/open/359_aarch64_recursive_stack_preserved_pointer_formal_post_call_overwrite.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The Stack-Preserved Pointer Formal Boundary

# Current Packet

## Just Finished

Lifecycle switched from parked idea 358 to split idea 359. Implementation has
not started for the new active plan.

## Suggested Next

Localize the stack-preserved pointer formal `%p.spare` overwrite/reload
boundary in prepared BIR and generated AArch64 before editing code.

## Watchouts

- The inherited first bad fact is `%p.spare` stack-home corruption from
  clobbered `x3` after a call, followed by a later recursive/helper-call reload
  from that corrupted slot.
- Do not reopen scalar `%p.n` post-call reloads unless fresh evidence shows
  the idea 358 repair regressed.
- Do not reopen callee-saved pointer-formal home publication from idea 357
  unless fresh evidence shows that boundary regressed.

## Proof

No implementation proof has been run for idea 359 yet.
