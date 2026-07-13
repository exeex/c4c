# Backend Test Source Reachability Cleanup

Status: Open
Type: backend test inventory and dead-source cleanup

## Intent

Audit every backend test `.cpp` against the supported build and test graphs,
then remove unreachable test sources and their dead support surface.  Future
agents should be able to treat the remaining backend test inventory as real
protection rather than mistake zombie files for compiled, registered, or
persistently executed coverage.

## Why This Exists

A directory listing alone cannot reveal registrations removed without their
sources, sources removed without their helpers, renamed or moved leftovers,
or older unregistered files disconnected from the supported graph.  The
cross-configuration build and test graphs are therefore the reachability
authority, supplemented by focused Git history only where it helps explain a
specific leftover or move.

## Required Inventory Method

1. Enumerate backend test `.cpp` files and the CMake sources and registrations,
   options and gates, helpers and fixtures, generated expectations, and
   ownership or documentation references that make up their supported graph.
2. Reconcile the filesystem inventory of backend test `.cpp` files
   against CMake target source membership, `add_test`/CTest registration,
   conditional options and architecture gates, and every supported persistent
   or opt-in test path.
3. Evaluate reachability across every supported configuration.  Distinguish
   intentionally build-only contract binaries from truly unreachable files;
   absence from one default configuration is not evidence that an optional but
   supported target is dead.
4. Use focused Git history when needed to explain suspected stale registrations,
   sources, support files, moves, or renames; no fixed lifecycle range is a
   prerequisite for the audit.
5. Remove only sources proven unreachable from every supported graph, together
   with dead CMake entries, helpers, fixtures, generated expectations, and
   ownership or documentation references that exist only for those sources.
6. Add a machine-checkable inventory or guard that reports unexplained backend
   test `.cpp` files outside all supported build/test graphs and prevents the
   zombie-source condition from recurring.

## In Scope

- Backend test `.cpp` files that are unreachable from every supported build or
  test configuration.
- Now-dead CMake source entries, CTest registrations, conditional gates,
  helpers, fixtures, generated expectations, and ownership or documentation
  references required for a clean removal.
- Build-only backend contract binaries whose intentional status must be
  represented explicitly in the inventory rather than inferred as dead.
- A reproducible reachability check covering the supported configuration and
  test matrix.

## Out Of Scope

- Backend semantic implementation changes.
- Deleting optional tests or architecture targets that remain supported but
  are omitted by one normal/default configuration.
- Weakening registrations, options, persistent baseline coverage, boundary
  contracts, or expected behavior to make a source appear unused.
- Reworking established LIR-to-BIR or BIR-to-MIR contract policy.
- Broad test redesign beyond removal of proven unreachable surface.

## Acceptance Criteria

- The audit records a reproducible inventory of backend test `.cpp` files and
  their supported build, CTest, persistent-test, or intentional build-only
  reachability edges.
- Every remaining backend `*_test.cpp` is explained by at least one supported
  compiled target, CTest execution path, persistent-baseline path, or explicit
  intentional build-only contract role.
- No backend test `.cpp` remains unreachable from all supported build/test
  graphs, and no removed test leaves stale CMake, helper, fixture, generated
  expectation, ownership, or documentation references.
- A machine-checkable inventory/guard fails when a new unexplained backend test
  source is added or when its last supported graph edge disappears.
- Clean configure, build, and CTest discovery pass for the supported
  configuration matrix, and existing persistent test paths remain non-empty
  and green where applicable.

## Reviewer Reject Signals

- The audit relies only on a directory listing or filename pattern without
  reconciling the supported build and test graphs.
- Directory names, filename patterns, or one default configuration are used as
  the sole evidence that a test is unreachable.
- An optional but supported test, architecture target, or intentionally
  build-only contract binary is deleted because it is absent from one baseline
  configuration.
- Registrations, options, architecture gates, baseline suite counts, expected
  results, or supported classifications are weakened to make files appear
  unused or to obtain green proof.
- Dead sources are deleted while stale CMake entries, helpers, fixtures,
  generated expectations, moves/renames, or ownership/documentation references
  remain unexplained.
- Testcase-shaped exceptions, allowlists, helper renames, expectation-only
  rewrites, or classification-only changes are presented as cleanup progress.
- Backend semantic changes, broad contract rewrites, or the old zombie-source
  failure mode behind a renamed inventory abstraction are mixed into the work.
