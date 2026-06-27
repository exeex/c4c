# Current Packet

Status: Active
Source Idea Path: ideas/open/414_typed_prepared_call_argument_contracts.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Existing Call-Argument Routes

## Just Finished

Lifecycle activation created the active runbook for Step 1.

## Suggested Next

Begin Step 1 by mapping the existing `PreparedCallArgumentSourceSelection`
routes and selecting the first route to migrate.

## Watchouts

- Do not add new optional fields to `PreparedCallArgumentSourceSelection`.
- Do not infer argument homes in RV64/AArch64 when producer facts are absent.
- Do not use named torture-file handling or expectation weakening as progress.

## Proof

Lifecycle-only activation; no build proof required.
