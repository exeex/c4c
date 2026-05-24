Status: Active
Source Idea Path: ideas/open/prealloc-responsibility-map-and-layout-plan.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Prealloc Files And Inputs

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from
`ideas/open/prealloc-responsibility-map-and-layout-plan.md`.

## Suggested Next

Execute Step 1 from `plan.md`: inventory `src/backend/prealloc`, collect file
sizes and broad surfaces, and record the audit output here.

## Watchouts

- Keep the first packet audit-only; do not move implementation files.
- Preserve target-specific instruction emission and register spelling outside
  prealloc.
- Create separate follow-up ideas for semantic ownership issues instead of
  mixing them into layout cleanup.

## Proof

Activation-only lifecycle change. Run `git diff --check` before supervisor
commit.
