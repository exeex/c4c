Status: Active
Source Idea Path: ideas/open/312_rv64_local_stack_slot_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Normalize Local Address Evidence

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from
`ideas/open/312_rv64_local_stack_slot_address_materialization.md`.

## Suggested Next

Start Step 1 by reproving and classifying the primary local stack/address
representatives: `src/00005.c`, `src/00032.c`, `src/00072.c`, `src/00130.c`,
and residual overlap probe `src/00143.c`.

## Watchouts

- Do not special-case candidate filenames, variable names, or fixed stack
  offsets.
- Keep external-call policy and global storage flow out of this plan unless
  needed only to isolate a local materialization failure.
- Treat `src/00143.c` as in-scope only if evidence shows the active blocker is
  local array/address materialization rather than switch flow or another
  separate mechanism.

## Proof

Lifecycle-only activation. No build or test run was required.
