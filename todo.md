Status: Active
Source Idea Path: ideas/open/141_prepared_lookups_residual_owner_audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory residual prepared lookup public groups

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from
`ideas/open/141_prepared_lookups_residual_owner_audit.md`.

## Suggested Next

Execute Step 1 from `plan.md`: inventory every remaining public group in
`src/backend/prealloc/prepared_lookups.hpp`, group declarations by semantic
purpose, and record unresolved ownership questions in this file.

## Watchouts

- This active idea is analysis-only; do not edit implementation files.
- Do not use line count alone as evidence that a residual group should move.
- Do not propose deleting reusable prepared facts or replacing them with local
  BIR rescans, predecessor rescans, or name matching.
- Keep target-local AArch64 routing, register, scratch, hazard, and emission
  policy out of shared prealloc follow-up proposals.
- Do not reopen ideas 137-140 unless a concrete residual dependency proves an
  unresolved owner boundary.

## Proof

Step 1 is inspection-only; no build proof is required.
