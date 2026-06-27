# Current Packet

Status: Active
Source Idea Path: ideas/open/415_prepared_value_materialization_contracts.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Current Materialization Recovery Boundaries

## Just Finished

Activated Idea 415 as the active plan and created the value materialization
contracts runbook.

## Suggested Next

Begin Step 1 by mapping current `RematerializableImmediate`,
`PointerBasePlusOffset`, and same-block producer-chain recovery boundaries.

## Watchouts

- Do not add target-local evaluators for arbitrary BIR expressions.
- Do not weaken runtime comparison, gcc_torture, or supported-path tests.
- Treat diagnostics-only changes that leave target recovery intact as
  incomplete.

## Proof

Lifecycle-only activation; proof is `git diff -- plan.md todo.md`.
