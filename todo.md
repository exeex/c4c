Status: Active
Source Idea Path: ideas/open/309_rv64_aggregate_global_storage.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish Aggregate Global Storage Baseline and Scope

# Current Packet

## Just Finished

Activation initialized `plan.md` and `todo.md` for idea 309. No executor packet
has run yet.

## Suggested Next

Delegate Step 1 to establish the aggregate global storage baseline, identify
the first focused backend contract, and choose the narrow proof command.

## Watchouts

- Do not repair the shared `.text`-only output contract in this plan.
- Do not special-case `src/00024.c`, symbol `v`, or fixed offsets `0` and `4`.
- Keep pointer globals, floating globals, strings, and external calls outside
  the acceptance claim unless the supervisor opens a separate scope.

## Proof

Lifecycle activation only; no build or test proof required for this slice.
