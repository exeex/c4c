Status: Active
Source Idea Path: ideas/open/179_bir_return_chain_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Existing Consumers

# Current Packet

## Just Finished

Lifecycle activation created the active runbook for Step 1.

## Suggested Next

Start Step 1 by mapping AArch64 prepared return-chain helper call sites and the
BIR route 8 context each migrated consumer needs.

## Watchouts

- Keep AArch64 target policy out of BIR.
- Do not contract prepared return-chain APIs in this plan.
- Treat prepared helper reads as diagnostics or bounded fallback only after the
  matching consumer migrates.
- Preserve fail-closed behavior for missing, invalid, or conflicting BIR route
  answers.

## Proof

Lifecycle-only activation; no build or test proof required.
