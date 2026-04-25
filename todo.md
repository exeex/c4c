Status: Active
Source Idea Path: ideas/open/115_hir_to_lir_layout_legacy_decl_demotion.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory HIR-to-LIR Legacy Layout Authority

# Current Packet

## Just Finished

Activation created the active runbook from Step 1 of `plan.md`; no executor
packet has run yet.

## Suggested Next

Execute Step 1 by inventorying HIR-to-LIR `StructuredLayoutLookup` consumers,
classifying remaining `legacy_decl` reads, and selecting the first narrow
code-changing demotion target.

## Watchouts

- Keep `legacy_decl` fallback for incomplete structured coverage.
- Do not edit `src/backend/mir/` as part of this route.
- Do not weaken tests or add testcase-shaped shortcuts.
- Record structured-vs-legacy mismatches in this file during execution.

## Proof

Not run. Activation is lifecycle-only.
