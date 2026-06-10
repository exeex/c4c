# Current Packet

Status: Active
Source Idea Path: ideas/open/156_bir_cfg_edge_publication_identity.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect the prepared edge-source oracle

## Just Finished

Lifecycle activation created this execution state for Step 1. No implementation
packet has run yet.

## Suggested Next

Delegate Step 1 to inspect the prepared edge-source oracle and classify the
semantic fields that BIR may own versus prealloc/codegen routing fields that
must stay out of BIR.

## Watchouts

- Keep parallel-copy scheduling, move execution, destination registers,
  storage-sharing checks, and prepared move records outside BIR.
- Treat the prepared publication helpers as the oracle until BIR equivalence is
  proven.
- Do not weaken tests or add named-case shortcuts to claim progress.

## Proof

No proof run for lifecycle-only activation.
