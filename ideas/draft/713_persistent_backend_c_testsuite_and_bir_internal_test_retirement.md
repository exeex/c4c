# Persistent Backend C Testsuite And BIR Internal Test Retirement

Status: Open
Type: backend baseline and test-surface cleanup
After: `ideas/open/712_route_debug_and_test_vocabulary_cleanup.md`

## Intent

Make the AArch64 and RV64 C testsuites part of the persistent baseline, then
retire BIR tests whose observable contract is an internal preparation,
preallocation, route, lookup, printer, dump, value-ID, slot-ID, or other
refactorable implementation detail.  Future BIR restructuring should be
protected by stable boundary contracts and executable backend behavior rather
than snapshots of every intermediate stage.

## Why This Exists

Ideas 703 through 712 establish the named LIR-to-BIR and BIR-to-MIR contract
surfaces.  Keeping duplicate structural oracles beneath those boundaries
locks later BIR refactors to the old representation without adding equivalent
semantic protection.  Always-on target C testsuites should own executable
behavior, while focused boundary tests should own the stable handoff rules.

## In Scope

- Register and execute both the AArch64 and RV64 C testsuites in the normal
  persistent baseline flow, with failures visible to baseline comparison.
- Prove that the baseline cannot silently omit, skip, disable, or tolerate
  regressions in either architecture suite.
- Inventory BIR and adjacent backend tests by the contract they protect.
- Preserve focused stable LIR-to-BIR contracts and named BIR-to-MIR contracts
  established by ideas 703 through 712.
- Preserve validator invariants and necessary malformed-input, rejection, and
  fail-closed coverage that downstream execution cannot safely prove.
- After the dual-architecture baseline gate is proven, delete duplicate tests
  and fixtures whose primary oracle is internal BIR stage structure, including
  prepare/prealloc/route/lookup/printer/dump details and unstable IDs or slots.
- Remove obsolete test registration, options, helpers, fixtures, and generated
  expectations that exist only for the retired internal tests.

## Out Of Scope

- Changing supported language or backend semantics.
- Weakening, reclassifying, or deleting stable boundary contracts, validator
  invariants, or malformed-input/fail-closed behavior.
- Treating debug dumps or internal representations as new public contracts.
- Rewriting BIR or target lowering merely to make the test retirement easier.

## Required Sequence And Proof Hierarchy

1. Establish a reproducible persistent baseline that continuously configures,
   registers, and runs both AArch64 and RV64 C testsuites.
2. Demonstrate that a failure or missing registration in either suite makes
   baseline comparison fail.  Record suite counts and skip/disable state so an
   empty or partially omitted suite cannot appear green.
3. Classify existing tests as stable LIR-to-BIR contract, named BIR-to-MIR
   contract, validator/fail-closed coverage, downstream executable behavior,
   or removable internal structural coverage.
4. Only after steps 1 and 2 are accepted, retire the internal structural
   tests in reviewable batches.  Each batch must retain the relevant boundary
   proof and pass both architecture C testsuites.

The proof hierarchy is: stable boundary contracts for handoff semantics;
validator and fail-closed tests for invalid states; always-on AArch64 and RV64
C testsuites for executable behavior.  Intermediate BIR representation
snapshots are not an additional required proof layer.

## Acceptance Criteria

- The normal persistent baseline executes non-empty AArch64 and RV64 C
  testsuites and fails on suite failure, omission, unexpected skip, or disable.
- Baseline before/after evidence shows both architecture suites remain
  continuously covered throughout test retirement.
- Remaining BIR-facing tests are limited to stable LIR-to-BIR contracts,
  named BIR-to-MIR contracts from the 703--712 series, and justified
  validator/malformed-input/fail-closed invariants.
- Internal prepare, prealloc, route, lookup, printer, dump, value-ID, slot-ID,
  and equivalent representation-shaped tests and their dead harness surface
  are removed when their behavior is covered by the proof hierarchy.
- No supported behavior or boundary contract is weakened to obtain a green
  result, and the resulting suite permits internal BIR representation changes
  without expectation churn when boundary semantics stay unchanged.

## Reviewer Reject Signals

- Any internal-test deletion begins before both architecture C testsuites are
  continuously registered, executed, non-empty, and baseline-enforced.
- A suite is disabled, allowlisted, broadly skipped, marked unsupported, or
  reduced to named cases while being claimed as persistent baseline coverage.
- Expected results or classifications are weakened instead of repairing a
  real backend regression exposed by the new baseline.
- Stable LIR-to-BIR or named BIR-to-MIR contracts, validator invariants, or
  necessary malformed-input/fail-closed tests are deleted as “internal.”
- Internal snapshots survive under renamed helpers, fixtures, dumps, or
  abstractions while retaining the same representation-shaped failure mode.
- Testcase-shaped shortcuts, expectation-only rewrites, or classification-only
  changes are claimed as capability or coverage progress.
- Broad BIR or lowering rewrites are mixed into the baseline/test-retirement
  work, or executable coverage is inferred from a narrow target testcase
  instead of both full persistent architecture suites.
