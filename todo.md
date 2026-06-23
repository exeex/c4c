Status: Active
Source Idea Path: ideas/open/323_rv64_loop_carried_pointer_postincrement_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Normalize Loop-Carried Pointer Evidence

# Current Packet

## Just Finished

Lifecycle activation created this executor-compatible todo skeleton for Step 1
of `plan.md`.

## Suggested Next

Start Step 1 by reproving `src/00143.c`, confirming the previously closed
boundaries still advance, and classifying the first loop-carried pointer
post-increment publication/lowering failure.

## Watchouts

- Do not special-case `src/00143.c`, Duff's-device block layout, `%lv.from`,
  `%lv.to`, `%t29`, fixed stack offsets, block names, or fixed array sizes.
- Do not avoid repeated fallthrough loop iterations in the focused fixture just
  to make proof pass.
- Do not reopen empty-exit successor, i16 halfword local-array, stack-homed
  fused compare, or nested store-source routes unless fresh evidence proves a
  regression.
- Keep runtime proof on the supported path; do not mark the candidate
  unsupported or skip qemu to claim progress.

## Proof

Lifecycle-only activation. No build or tests were run.
