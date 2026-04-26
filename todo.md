Status: Active
Source Idea Path: ideas/open/116_bir_layout_dual_path_coverage_and_dump_guard.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory BIR Aggregate Layout Authority

# Current Packet

## Just Finished

Activation created this executor-compatible state for `plan.md` Step 1.

## Suggested Next

Begin Step 1 by inventorying BIR aggregate layout authority in
`src/backend/lir_to_bir.cpp`, backend layout helpers, and relevant aggregate
tests. Record structured-ready, parity-gated, and fallback-only paths here
before selecting the first implementation target.

## Watchouts

- Keep legacy `type_decls` available as fallback and parity evidence.
- Do not remove legacy parsing helpers during this runbook.
- Treat `--dump-bir` tests as guards for lowered BIR facts, not as a BIR
  printer render-context migration.
- Do not downgrade expectations or add named-case shortcuts.

## Proof

No validation run for lifecycle-only activation.
