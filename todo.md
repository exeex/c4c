Status: Active
Source Idea Path: ideas/open/327_aarch64_fixed_formal_entry_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Fixed Formal Entry Publication Gap

# Current Packet

## Just Finished

Lifecycle switch completed from idea 326 to idea 327 after Step 4 classified
the `00204.c` segfault as an adjacent fixed-formal entry publication issue.

## Suggested Next

Execute Step 1 from `plan.md`: localize `%p.format` from semantic BIR formal
through prepared AArch64 storage, ABI incoming `x0`, assigned prepared home,
and first generated consumer before implementing a repair.

## Watchouts

- Do not special-case `00204.c`, `myprintf`, `%p.format`, `x0`, `x13`, one
  register, one stack slot, one pointer type, or one emitted instruction
  sequence.
- Do not reopen HFA/floating `va_arg`, global initializer, fixed HFA argument,
  fixed HFA return, local/value-home, frame-layout, runner, expectation,
  unsupported, or proof-log behavior without fresh generated-code evidence.
- Preserve idea 326's parked state; resume it only after fixed-formal entry
  publication is repaired and fresh representative evidence returns to an
  HFA/floating owner.

## Proof

Lifecycle-only update. No build or CTest proof was run.
