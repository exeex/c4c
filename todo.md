Status: Active
Source Idea Path: ideas/open/314_rv64_aggregate_local_subobject_and_byval_flow.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Normalize Aggregate Residual Evidence

# Current Packet

## Just Finished

Lifecycle activation created this executor-compatible todo skeleton for Step 1
of `plan.md`.

## Suggested Next

Start Step 1 by reproving `src/00019.c`, `src/00046.c`, and `src/00140.c`,
capturing emitted-code evidence, and classifying the first bad runtime
mechanism for each candidate.

## Watchouts

- Do not special-case filenames, fixed offsets, or struct/union names.
- Do not reopen generic local frame-slot address publication unless aggregate
  evidence proves it is still incomplete.
- Keep broad vararg and floating aggregate ABI repair out of scope unless it is
  only being classified as a residual.

## Proof

Lifecycle-only activation. No build or tests were run.
