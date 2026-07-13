# Backend Test Contract Surface Retirement Runbook

Status: Active
Source Idea: ideas/open/714_backend_test_source_reachability_cleanup.md

## Purpose

Retain only backend tests whose direct observable contract is the stable
LIR-to-BIR or BIR-to-MIR interface, and retire the remaining backend test and
support surface.

## Goal

Produce an assertion-level classification, enforce the two-interface-only
policy, remove non-boundary tests in reviewable batches, clean their dedicated
support, and establish a new baseline from the retained surface.

## Core Rule

Retain a test only when its direct observable assertions specify LIR-to-BIR or
BIR-to-MIR.  Build reachability and current pass/fail status are not retention
criteria.

## Read First

- `ideas/open/714_backend_test_source_reachability_cleanup.md`
- LIR-to-BIR public handoff and its focused tests
- BIR-to-MIR public handoff and its focused tests
- backend test targets, registrations, fixtures, helpers, and expectations
- current baseline evidence as classification input only

## Current Scope

- Every backend test source and its direct assertions.
- Direct stable LIR-to-BIR and BIR-to-MIR boundary tests.
- All non-boundary backend tests and their dedicated support/build surface.
- A guard enforcing evidence-backed classification into the two retained
  interfaces.
- Final focused validation and supervisor-owned baseline establishment.

## Non-Goals

- Do not change backend semantics or redesign the retained interfaces.
- Do not repair target lowering, emission, object, or runtime behavior solely
  to preserve a test slated for retirement.
- Do not weaken, rename, or relabel retained boundary expectations.
- Do not preserve a test merely because it is reachable, useful, green, or
  currently failing.

## Working Model

- `retain: LIR-to-BIR` means the test directly observes the stable LIR input to
  BIR boundary contract.
- `retain: BIR-to-MIR` means the test directly observes the stable BIR input to
  MIR boundary contract.
- `retire` covers every other direct contract, including internal stages and
  downstream target/runtime behavior.
- Validator or fail-closed coverage follows the same direct-interface rule; it
  is not a third retained category.

## Execution Rules

- Review assertions, not only filenames, targets, or registrations.
- Record evidence for each retained classification and the dedicated support
  ownership of each retirement batch.
- Explicitly retire route, prepared/prealloc, dump, printer, lookup, value-ID,
  slot-ID, target lowering/emission, object, and runtime tests unless a specific
  assertion is necessary direct proof of a retained interface.
- Treat baseline failures as classification evidence, not automatic repair
  work.
- Keep deletions reviewable and validate both retained interfaces after each
  batch that can affect shared support.
- The supervisor establishes the new baseline only after accepted deletion
  batches and final retained-surface validation.

## Step 1: Inventory assertions and classify the test surface

Goal: Classify every backend test by its direct observable contract.

Concrete actions:

- Enumerate backend test sources, targets, registrations, fixtures, helpers,
  expectations, and options or gates.
- Inspect each test's direct assertions and classify it as `LIR-to-BIR`,
  `BIR-to-MIR`, or `retire`.
- Require assertion-level evidence for every retained test.
- Classify validator, malformed, rejection, and fail-closed cases by the same
  two-interface rule.
- Identify dedicated and shared support dependencies for deletion planning.
- Use existing baseline failures only as evidence about exercised contracts.

Completion check:

- Every backend test has one classification, every retention has direct
  boundary evidence, and deletion/support batches are explicit.

## Step 2: Add the two-interface classification guard

Goal: Prevent non-boundary backend tests from re-entering the test surface.

Concrete actions:

- Implement a machine-checkable inventory or guard for the two retained
  interface categories.
- Require an evidence-backed category for every backend test admitted by the
  guard.
- Prove that an unclassified or non-boundary representative test is rejected.
- Avoid filename-shaped allowlists and named-test exceptions.

Completion check:

- The guard accepts the reviewed two-interface inventory and rejects
  representative non-boundary or unclassified additions.

## Step 3: Retire internal-stage tests in reviewable batches

Goal: Remove tests whose contracts expose refactorable backend internals.

Concrete actions:

- Delete classified route, prepared/prealloc, internal dump, printer, lookup,
  value-ID, and slot-ID tests in coherent batches.
- Include other internal-stage tests whose direct assertions specify neither
  retained interface.
- Remove batch-dedicated expectations and support when ownership is exclusive;
  defer shared-support cleanup to Step 5.
- Build and run focused LIR-to-BIR and BIR-to-MIR proof after relevant batches.

Completion check:

- The classified internal-stage surface is gone without weakening or
  reclassifying a retained boundary expectation.

## Step 4: Retire downstream target and runtime tests in reviewable batches

Goal: Remove tests whose direct contracts are beyond BIR-to-MIR.

Concrete actions:

- Delete classified target lowering, target emission, object production, and
  backend runtime tests in coherent batches.
- Do not repair current downstream failures merely to keep retiring tests.
- Preserve a particular assertion only when review proves it is necessary
  direct LIR-to-BIR or BIR-to-MIR evidence.
- Run focused proof for both retained interfaces after shared-surface changes.

Completion check:

- No downstream target/object/runtime test remains without concrete direct
  retained-interface evidence.

## Step 5: Remove dedicated support and build/test registrations

Goal: Remove infrastructure that exists only for retired tests.

Concrete actions:

- Remove unused fixtures, helpers, generated expectations, build targets,
  CTest registrations, options, and gates.
- Confirm shared support remains wherever a retained boundary test uses it.
- Search for stale source, target, registration, expectation, and ownership
  references.
- Run clean build and focused boundary-test discovery.

Completion check:

- No dedicated retired-test support or stale registration remains, and both
  retained interface suites still build and register as intended.

## Step 6: Validate retained interfaces and establish the new baseline

Goal: Prove the final two-interface surface and hand baseline acceptance to the
supervisor.

Concrete actions:

- Run focused LIR-to-BIR and BIR-to-MIR validation with non-empty discovery
  evidence for both categories.
- Run the classification guard against the final test inventory.
- Run the supervisor-selected broader regression check appropriate to shared
  implementation/build changes.
- Review deletion batches and confirm retained expectation strength did not
  change.
- Have the supervisor establish and record a new accepted baseline from the
  retained surface.

Completion check:

- Only direct tests of the two stable interfaces remain, their focused proof
  and guard are green, deletion/support cleanup is complete, and the new
  retained-surface baseline is accepted without silently omitting either
  interface.
