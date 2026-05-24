Status: Active
Source Idea Path: ideas/open/aarch64-codegen-reference-layout-consolidation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map AArch64 Codegen Files

# Current Packet

## Just Finished

Lifecycle activation created `plan.md` and `todo.md` for Step 1 of the AArch64
codegen reference layout consolidation runbook.

## Suggested Next

Execute Step 1: list every direct `.cpp` and `.hpp` file under
`src/backend/mir/aarch64/codegen/`, map each file to one reference family or
`adapter/internal`, and record the prioritized consolidation list here.

## Watchouts

- Step 1 is audit-only; do not edit implementation files while producing the
  initial map.
- Do not treat this as a prealloc, Prepared, BIR, or forward-migration audit.
- Do not weaken tests, downgrade supported behavior, or create testcase-shaped
  shortcuts in later implementation packets.
- If a target-neutral migration candidate appears, record it as a separate open
  idea instead of expanding this layout cleanup.

## Proof

No validation run for lifecycle-only activation.
