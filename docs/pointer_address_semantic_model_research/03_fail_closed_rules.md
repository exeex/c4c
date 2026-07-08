# 03. Fail-Closed Rules

Status: Step 1 skeleton

## Assigned Purpose

Define rejectable pointer/address evidence failures without relying on
target-local inference or diagnostic-only facts.

## Required Failure Modes

- Missing evidence.
- Ambiguous evidence.
- Stale evidence.
- Wrong value.
- Wrong use.
- Stack-home-only evidence.
- Local-layout-only evidence.
- Relocation-less evidence.
- Target-shape-only evidence.

## Required Output Shape

For each surveyed family, record:

| Family | Failure mode | Required fail-closed behavior | Current evidence surface | Later proof surface |
| --- | --- | --- | --- | --- |
| TBD | TBD | TBD | TBD | TBD |

## Step 1 Boundary

This skeleton sets the fail-closed answer shape only. It intentionally does
not decide Step 4 rules.
