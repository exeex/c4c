# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory TypeSpec Tag Surfaces

## Just Finished

Activated `ideas/open/141_typespec_tag_field_removal_metadata_migration.md`
into `plan.md`.

## Suggested Next

Start Step 1 by inventorying `TypeSpec::tag` reads/writes and related metadata
carriers across parser, Sema, HIR, and lowering. Classify each surface before
attempting field deletion, and name the first concrete migration target.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic field.
- Preserve diagnostics, dumps, mangling, ABI/link-visible text, and final
  spelling as payloads.
- Do not weaken tests, mark supported cases unsupported, or add named-case
  shortcuts.
- Keep downstream LIR/BIR/backend carrier gaps as separate follow-up ideas
  instead of broadening this runbook.
- Treat a `TypeSpec::tag` deletion build as a temporary probe only until the
  runbook reaches the removal step.

## Proof

Activation-only lifecycle update; no build or test validation was run.
