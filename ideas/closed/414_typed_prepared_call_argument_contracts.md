# Typed Prepared Call Argument Contracts

Status: Open
Type: Follow-up contract refactor idea
Parent: `ideas/closed/412_prepared_fact_contract_normalization_analysis.md`
Depends On: initial taxonomy from `ideas/open/413_prepared_contract_verifier_and_owner_taxonomy.md` and target-consumer findings from `ideas/open/418_prepared_target_consumer_boundary_audit.md`
Handoff Inputs: `docs/prepared_fact_contracts/contract_taxonomy_and_fact_family_matrix.md`, `docs/prepared_fact_contracts/target_consumer_boundary_audit.md`

## Goal

Replace the optional-field call-argument source-selection bag with typed
prepared call-argument route contracts.

## Why This Exists

`PreparedCallArgumentSourceSelection` uses a `kind` plus many unrelated optional
fields for prior preservation, frame-slot address/value, local address
materialization, and byval lanes. Ideas 403 and 407 showed that scalar ABI/home
facts belong to prepared producers; target consumers must not infer missing
argument homes from formal order, type, or callee facts.

## In Scope

- Define typed payloads for existing source-selection routes.
- Use the early target-consumer audit to identify which RV64/AArch64 call
  argument checks are recovering producer facts and should be blocked by this
  typed contract.
- Cite the consumed taxonomy and target-consumer audit rows in
  `docs/prepared_fact_contracts/call_argument_contract_plan.md`.
- Add compatibility accessors or an incremental bridge so the migration can be
  staged.
- Add verifier rules for required fields per route.
- Update RV64/AArch64 consumers to exhaustively consume typed routes where the
  migrated family is enabled.
- Add focused tests for same-module subword scalar arguments and existing
  preservation/frame-slot routes.

## Out Of Scope

- Changing ABI policy for unrelated aggregate or variadic calls.
- Adding target-specific inference when a typed route is absent.
- Rewriting all call lowering in one slice.
- Making RV64 gcc_torture monotonic pass count the acceptance gate.

## Acceptance Criteria

- Migrated call-argument routes cannot be represented with contradictory
  optional-field combinations.
- The call-argument contract plan names the idea 413/418 rows consumed by this
  slice, or explicitly records that no applicable target-consumer audit row
  exists for the selected route.
- Missing producer call-argument facts fail before target consumers try to
  recover them.
- Default `ctest --test-dir build -j --output-on-failure` / normal CTest does
  not regress.
- RV64 gcc_torture representatives may temporarily regress only as precise
  fail-closed contract diagnostics, not silent miscompiles.

## Reviewer Reject Signals

- Reject adding more optional fields to `PreparedCallArgumentSourceSelection`
  instead of typed payloads for the selected route.
- Reject RV64/MIR code that guesses same-module argument destinations, formal
  homes, stack offsets, or register banks when producer facts are absent.
- Reject testcase-specific handling for `divmod-1.c`, `20000403-1.c`, or other
  named torture files.
- Reject expectation rewrites, allowlist filtering, or weaker test contracts.
- Reject a route that leaves the old contradictory optional combinations
  accepted behind a renamed API.

## Closure Note

Closed after migrating the prepared call-argument typed routes covered by this
idea through `FrameSlotAddress`, `FrameSlotValue`, and
`LocalFrameAddressMaterialization`. The final route added a typed payload and
bridge accessor, producer-side verifier statuses/reports, RV64/AArch64
consumer migration, coherent prepared-fixture updates, and broad validation.

Close proof: `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure` passed 3356/3356 tests, and the close-scope regression
guard passed against the accepted 3356/3356 full-suite baseline.
