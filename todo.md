Status: Active
Source Idea Path: ideas/open/324_rv64_duff_fallthrough_pointer_update_producers.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Normalize Duff Fallthrough Producer Evidence

# Current Packet

## Just Finished

Lifecycle activation created this executor-compatible todo skeleton for Step 1
of `plan.md`.

## Suggested Next

Start Step 1 by reproving `src/00143.c`, confirming the idea 323 current-pointer
post-increment path still advances, and classifying the first later Duff
fallthrough pointer update with missing producer facts.

## Watchouts

- Do not special-case `src/00143.c`, Duff's-device block names, `%t39`,
  `%t42`, `%lv.from`, `%lv.to`, fixed stack offsets, or fixed array sizes.
- Do not teach RV64 to guess from uninitialized stack homes or synthesize
  pointer updates without prepared semantic authority.
- Do not reopen idea 323's RV64 consumption path while later fallthrough blocks
  still have unknown source producers.
- Keep runtime proof on the supported path; do not weaken qemu coverage or
  mark the candidate unsupported.

## Proof

Lifecycle-only activation. No build or tests were run.
