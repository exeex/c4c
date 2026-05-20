Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Current HFA/Floating Residual

# Current Packet

## Just Finished

Lifecycle handoff closed idea 330 and activated idea 326. Execution has not
started on the new active packet.

## Suggested Next

Start Step 1 by localizing the first current HFA/floating or long-double bad
branch in generated `myprintf` after the repaired `%7s` / `%9s` non-HFA
aggregate string `va_arg` branches.

## Watchouts

Do not reopen the closed non-HFA aggregate string materialization route unless
fresh generated-code evidence shows selected `%7s` / `%9s` bytes are again
missing before their observing `printf` calls.

Preserve the prior-owner guardrails named in `plan.md`, especially idea 330
non-HFA aggregate `va_arg` copies and idea 329 post-`va_arg` call publication.

## Proof

No implementation proof has been run for the new active packet yet.
