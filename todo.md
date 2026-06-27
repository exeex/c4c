# Current Packet

Status: Active
Source Idea Path: ideas/open/415_prepared_value_materialization_contracts.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Pointer Materialization Boundaries

## Just Finished

Replaced the exhausted rematerializable integer immediate runbook with the next
same-source `PointerBasePlusOffset` materialization runbook.

Completed context from the prior runbook:

- Added and migrated typed rematerializable integer immediate facts.
- Added producer-side verification for the immediate family.
- Migrated selected RV64 consumers to the typed verifier/query.
- Full default CTest passed 3356/3356 with regression guard PASS.
- Corrected the typed immediate contract to accept prepared value id `0` as a
  valid first prepared value.

## Suggested Next

Begin Step 1 by mapping current `PointerBasePlusOffset` producers, payload
fields, target consumers, rejected combinations, and a focused Step 2 proof
command.

## Watchouts

- Do not add target-local evaluators for arbitrary pointer expressions.
- Do not weaken runtime comparison, gcc_torture, or supported-path tests.
- Treat diagnostics-only changes that leave target pointer recovery intact as
  incomplete.

## Proof

Lifecycle-only runbook replacement; no build required.
