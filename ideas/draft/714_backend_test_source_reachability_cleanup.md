# Backend Test Contract Surface Retirement

Status: Open
Type: backend test contract-surface cleanup

## Intent

Reduce the backend test surface to tests whose direct observable contract is
one of two stable compiler interfaces:

1. the LIR-to-BIR interface
2. the BIR-to-MIR interface

Retire other backend tests and the dedicated fixtures, helpers, expectations,
build targets, and registrations that exist only for them.  Reachability in the
current build graph, historical usefulness, or a current test failure does not
by itself justify preserving a test.

## Why This Exists

Backend tests currently protect a mixture of stable handoff contracts and
refactorable implementation stages or downstream execution details.  Keeping
all reachable tests turns internal routes, prepared forms, allocation choices,
dumps, target emission, and runtime behavior into accidental structural
contracts.  The retained surface should state only the two stable interface
contracts so backend internals and downstream implementation can evolve
without expectation churn at every intermediate layer.

## Retention Policy

A backend test survives only when its direct observable assertion specifies:

- LIR accepted by the LIR-to-BIR interface and the resulting stable BIR
  boundary contract; or
- BIR accepted by the BIR-to-MIR interface and the resulting stable MIR
  boundary contract.

Validator, malformed-input, rejection, and fail-closed tests survive only when
their direct observable contract is one of those two interfaces.  A test is not
retained merely because it is compiled, registered, reachable, useful for
debugging, currently green, or currently failing.

Do not weaken a retained boundary expectation, relabel an internal test as a
boundary test, or move its assertion behind a new helper merely to keep or
delete it.

## Required Classification And Retirement Method

1. Inventory every backend test source, target, and registration together with
   its direct observable assertions and dedicated support surface.
2. Classify each test as direct LIR-to-BIR contract, direct BIR-to-MIR
   contract, or retire.  Record concrete assertion evidence for every retained
   classification.
3. Explicitly classify for retirement tests of backend routes, prepared or
   preallocated forms, internal dumps, printers, lookups, value IDs, slot IDs,
   target lowering or emission, object production, and backend runtime
   behavior unless a particular assertion is necessary direct proof of one
   retained interface.
4. Add a machine-checkable guard that prevents backend tests outside the two
   retained interface categories from returning without an explicit,
   evidence-backed boundary classification.
5. Delete retired tests in reviewable batches, then remove their dedicated
   fixtures, helpers, generated expectations, targets, options, gates, and
   CTest registrations once no retained interface test uses them.
6. Validate the retained boundary surface and establish a new accepted
   baseline after the deletion batches are accepted.

## Baseline Policy

Existing baseline failures are classification inputs, not repair obligations.
They may reveal which contract a test exercises, but they do not require
repairing a test that the retention policy says to retire.  Do not weaken a
retained boundary expectation to obtain green proof.  After accepted deletion
batches, the supervisor establishes a new baseline from the retained
two-interface surface.

## In Scope

- Direct stable LIR-to-BIR and BIR-to-MIR boundary tests.
- Classification and deletion of all other backend tests.
- Retirement of route, prepared, preallocation, internal dump, printer,
  lookup, value-ID, slot-ID, target lowering/emission, object, and runtime
  backend tests outside the two direct interfaces.
- Dedicated fixtures, helpers, expectations, build targets, options, gates,
  and registrations used only by retired tests.
- A machine-checkable guard for the two-interface-only policy.
- Final validation and baseline establishment for the retained surface.

## Out Of Scope

- Changing compiler or backend semantics.
- Repairing target lowering, emission, object, or runtime behavior solely to
  keep a test that this policy retires.
- Weakening or renaming a retained interface contract.
- Preserving tests because they are reachable, longstanding, currently green,
  currently failing, or convenient debugging tools.
- Broad redesign of the two retained interfaces.

## Acceptance Criteria

- Every backend test is classified from its direct observable assertions, with
  retained tests limited to stable LIR-to-BIR or BIR-to-MIR contracts.
- Validator, malformed-input, rejection, and fail-closed coverage remains only
  where it directly specifies one retained interface.
- Route, prepared, preallocation, internal dump/printer/lookup, value-ID,
  slot-ID, target lowering/emission, object, and runtime backend tests are
  removed unless concrete evidence proves an assertion is necessary direct
  boundary proof.
- Dedicated support and build/test registrations for retired tests are removed
  without deleting support still used by a retained boundary test.
- A machine-checkable guard rejects backend tests that do not carry an
  evidence-backed classification under one of the two retained interfaces.
- Retained boundary expectations are unchanged in strength, and focused proof
  for both interfaces is green.
- The supervisor records a new accepted baseline after the deletion batches,
  derived from the retained surface rather than treating old failures as repair
  requirements.

## Reviewer Reject Signals

- Reachability, registration, current pass/fail status, filename, or historical
  usefulness is used as the primary retention rule instead of direct observable
  interface assertions.
- A route, prepared/prealloc, dump, printer, lookup, unstable-ID, target
  lowering/emission, object, or runtime test survives without concrete proof
  that its assertion directly specifies LIR-to-BIR or BIR-to-MIR.
- Validator, malformed-input, rejection, or fail-closed tests survive while
  specifying an internal or downstream contract rather than one retained
  interface.
- A retained boundary expectation is weakened, rewritten, reclassified, or
  hidden behind a renamed helper to make classification or validation pass.
- Tests are deleted by broad filename/category matching without assertion-level
  review, or dedicated support is removed while a retained test still uses it.
- Baseline failures trigger unrelated semantic repair, or old baseline counts
  are preserved by keeping tests outside the policy.
- The new baseline is accepted before deletion batches and retained-boundary
  validation are reviewed, or it silently omits either retained interface.
- Testcase-shaped exceptions, allowlists, or classification-only renames permit
  the old non-boundary surface to survive behind a new abstraction.
