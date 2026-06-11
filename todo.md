Status: Active
Source Idea Path: ideas/open/196_prepared_function_lookups_ownership_readiness_audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory lookup groups and readers

# Current Packet

## Just Finished

Lifecycle activation created this execution scratchpad for Step 1 of `plan.md`.

## Suggested Next

Start Step 1 by inventorying every `PreparedFunctionLookups` lookup group and
its readers. Keep production, printer/debug, target-wrapper, oracle-test, and
unknown readers separated.

## Watchouts

- Do not rename, split, delete, or privatize `PreparedFunctionLookups` fields
  under this audit.
- Do not classify target/prepared policy as BIR-owned just because a selected
  route consumer moved.
- Do not weaken oracle coverage, tests, unsupported markers, or expectations to
  make a lookup group appear ready.

## Proof

Lifecycle-only activation. No build or test proof was run.
