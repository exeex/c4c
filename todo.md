Status: Active
Source Idea Path: ideas/open/363_aarch64_prepared_select_condition_join_stale_reload.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The Stale Join Reload Boundary

# Current Packet

## Just Finished

Lifecycle switch from idea 362: pointer-derived load/address scaling is parked
after commit `321031ce0` because the source intent is satisfied but close-time
monotonic regression guard rejected archival closure. The next active residual
is split to idea 363.

## Suggested Next

Execute Plan Step 1. Localize the prepared select/condition join publication
boundary that lets the source and destination scan false exits materialize
zero/false and then reload stale stack slots at the join.

## Watchouts

- Preserve idea 362's address-scaling evidence: the index carrier and immediate
  scale must stay in distinct registers for pointer-derived offsets.
- Preserve idea 361's materialized pointer store writeback evidence.
- Preserve idea 360's correct starting-state evidence.
- Do not special-case `00181`, `Move`, Hanoi tower globals, block labels,
  stack offsets, or emitted registers.

## Proof

No implementation proof has been run for idea 363 yet. The previous lifecycle
decision used the existing focused logs and supervisor-reported backend guard:
focused proof after commit `321031ce0` was 6/7 with `00181` advanced from a
5 second timeout to fast `[RUNTIME_NONZERO] exit=Segmentation fault`, and the
supervisor backend guard passed 141/141.
