# Current Packet

Status: Active
Source Idea Path: ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory HIR Rendered Lookup Surfaces

## Just Finished

Lifecycle activation created this executor-compatible scratchpad for Step 1.

## Suggested Next

Start Step 1 by inventorying HIR rendered lookup surfaces and classifying each
retained string use before choosing the first migration target.

## Watchouts

- Do not edit parser/Sema, LIR, BIR, or backend code except to record a
  boundary or bridge a HIR-owned blocker required by the active plan.
- Do not weaken tests, mark supported routes unsupported, or add named-case
  shortcuts.
- Preserve diagnostics, dumps, mangling, ABI/link-visible text, and final
  spelling as payloads while removing rendered strings as semantic authority.

## Proof

No code proof required for lifecycle-only activation.
