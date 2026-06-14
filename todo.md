Status: Active
Source Idea Path: ideas/open/255_phase_f3_prepared_private_pass_context_metadata_gate.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Consumer Inventory

# Current Packet

## Just Finished

Activation created the runbook from idea 255. No implementation work has run
for this packet yet.

## Suggested Next

Start Step 1 by inventorying direct and indirect consumers of
`PreparedBirModule::route`, `PreparedBirModule::invariants`,
`PreparedBirModule::completed_phases`, and `PreparedBirModule::notes`.

## Watchouts

- Keep `PreparedBirModule::liveness` out of scope.
- Do not delete fields or weaken printer/status/debug/absent-note behavior.
- Mark target-facing public consumers as blockers instead of hiding them behind
  a renamed accessor.

## Proof

No proof run for lifecycle-only activation.
