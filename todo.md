Status: Active
Source Idea Path: ideas/open/111_store_source_publication_dump_visibility.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Existing Store-Source Facts And Dump Conventions

# Current Packet

## Just Finished

Activation created the active runbook and initialized executor state for
Step 1.

## Suggested Next

Execute Step 1 from `plan.md`: map the existing
`PreparedStoreSourcePublicationPlan` facts, locate the prepared dump surface,
and identify the focused prepared-printer fixture target before making
implementation edits.

## Watchouts

- Print existing prepared/publication-plan facts only; do not re-derive
  source-producer or select-chain facts in the printer.
- Keep output bounded to store-source publication-plan visibility.
- Tests must assert concrete source-producer and direct-global select-chain
  fields, not only a section header.
- Preserve missing-input fail-closed behavior and do not weaken expectations.

## Proof

Lifecycle-only activation; no code validation run.
