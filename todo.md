Status: Active
Source Idea Path: ideas/open/139_addressing_lookup_ownership_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect current addressing lookup ownership and dependencies

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from
`ideas/open/139_addressing_lookup_ownership_cleanup.md`.

## Suggested Next

Execute Step 1 from `plan.md`: inspect current addressing lookup ownership and
dependencies, then record declaration locations, definition locations,
constructor wiring, consumer include constraints, and the exact Step 2
declaration-boundary move in this file.

## Watchouts

- Do not edit implementation files during Step 1.
- Do not change address lowering, frame layout, TLS/global relocation behavior,
  or store-source publication semantics.
- Do not replace prepared addressing facts with local scans or name matching.
- Keep `PreparedFunctionLookups` cached aggregate access intact.

## Proof

Step 1 is inspection-only; no build proof is required until the first
code-changing packet.
