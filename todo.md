# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Classify Frontend/HIR Test Fixture Tag Uses

## Just Finished

Lifecycle activation from the parent field-removal idea after the codegen/LIR
child runbook was retired parked. The remaining deletion-probe boundary is
frontend/HIR test-fixture cleanup, not codegen/LIR aggregate identity work.

## Suggested Next

Execute Step 1 by classifying direct `TypeSpec::tag` reads and writes under
`tests/frontend`, starting from the latest probe residuals named in `plan.md`.
Pick one narrow fixture family for the first migration packet.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- Temporary deletion probes must be restored unless the packet is the final
  accepted field-removal deletion.

## Proof

Lifecycle-only activation. No code validation was run.
