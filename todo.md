# Current Packet

Status: Active
Source Idea Path: ideas/open/415_prepared_value_materialization_contracts.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Producer-Chain Boundaries

## Just Finished

Replaced the exhausted pointer base-plus-offset runbook with the next
same-source same-block producer-chain materialization runbook.

Completed context from prior runbooks:

- Added and migrated typed rematerializable integer immediate facts.
- Added and migrated typed pointer base-plus-offset facts.
- Added producer-side verification for both completed materialization
  families.
- Migrated selected RV64 consumers to typed verifier/query paths.
- Full default CTest passed 3356/3356 with regression guard PASS after the
  pointer family.

## Suggested Next

Begin Step 1 by mapping current same-block producer-chain recovery paths,
prepared producer lookup records, rejected combinations, and a focused Step 2
proof command.

## Watchouts

- Do not add target-local evaluators for arbitrary BIR expressions.
- Do not weaken runtime comparison, gcc_torture, or supported-path tests.
- Treat diagnostics-only changes that leave target producer-shape recovery
  intact as incomplete.

## Proof

Lifecycle-only runbook replacement; no build required.
