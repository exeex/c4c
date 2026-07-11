# Backend Test Source Reachability Cleanup

Status: Open
Type: backend test inventory and dead-source cleanup
After: `ideas/open/713_persistent_backend_c_testsuite_and_bir_internal_test_retirement.md`

## Intent

After idea 713 is complete, audit every remaining backend test `.cpp` against
the supported build and test graphs, then remove unreachable test sources and
their dead support surface.  Future agents should be able to treat the
remaining backend test inventory as real protection rather than mistake zombie
files for compiled, registered, or persistently executed coverage.

## Why This Exists

Idea 713 deliberately changes both the persistent baseline and the BIR test
surface.  A post-cleanup directory listing alone cannot reveal registrations
removed without their sources, sources removed without their helpers, renamed
or moved leftovers, or older unregistered files exposed by the final graph.
The complete 713 history diff is therefore the primary audit trail, with the
final cross-configuration build graph as the reachability authority.

## Required History And Inventory Method

1. Locate the idea-713 lifecycle history checkpoint in Git: use its activation
   or implementation start commit, or the nearest justified pre-713 completion
   base when activation is not represented by one commit.
2. Compare that checkpoint through the idea-713 completion commit.  Use `HEAD`
   only when it is the actual accepted completion point, and record the chosen
   commits and rationale so the audit is reproducible.
3. From that complete diff, enumerate removed and changed backend test sources,
   CMake sources and registrations, options and gates, helpers and fixtures,
   generated expectations, moves or renames, and ownership/documentation
   references.  Specifically look for registrations removed without sources,
   sources removed without registrations or support files, and leftovers from
   renamed or moved tests.
4. Reconcile the resulting filesystem inventory of backend test `.cpp` files
   against CMake target source membership, `add_test`/CTest registration,
   conditional options and architecture gates, and the persistent AArch64 plus
   RV64 baseline created by idea 713.
5. Evaluate reachability across every supported configuration.  Distinguish
   intentionally build-only contract binaries from truly unreachable files;
   absence from one default configuration is not evidence that an optional but
   supported target is dead.
6. Add a machine-checkable inventory or guard that reports unexplained backend
   test `.cpp` files outside all supported build/test graphs and prevents the
   zombie-source condition from recurring.

## In Scope

- Backend test `.cpp` files that are unreachable from every supported build or
  test configuration after idea 713.
- Now-dead CMake source entries, CTest registrations, conditional gates,
  helpers, fixtures, generated expectations, and ownership or documentation
  references required for a clean removal.
- Build-only backend contract binaries whose intentional status must be
  represented explicitly in the inventory rather than inferred as dead.
- A reproducible reachability check covering the final supported configuration
  matrix and idea-713 persistent baseline.

## Out Of Scope

- Backend semantic implementation changes.
- Deleting optional tests or architecture targets that remain supported but
  are omitted by one normal/default configuration.
- Weakening registrations, options, persistent baseline coverage, boundary
  contracts, or expected behavior to make a source appear unused.
- Reworking the LIR-to-BIR or BIR-to-MIR contract policy established by ideas
  703 through 713.
- Broad test redesign beyond removal of proven unreachable surface.

## Acceptance Criteria

- The recorded Git comparison spans the justified pre-713 lifecycle checkpoint
  through the accepted 713 completion commit and accounts for every relevant
  test-graph change in that diff.
- Every remaining backend `*_test.cpp` is explained by at least one supported
  compiled target, CTest execution path, persistent-baseline path, or explicit
  intentional build-only contract role.
- No backend test `.cpp` remains unreachable from all supported build/test
  graphs, and no removed test leaves stale CMake, helper, fixture, generated
  expectation, ownership, or documentation references.
- A machine-checkable inventory/guard fails when a new unexplained backend test
  source is added or when its last supported graph edge disappears.
- Clean configure, build, and CTest discovery pass for the supported
  configuration matrix, and idea 713's persistent AArch64 and RV64 C-testsuite
  baseline remains non-empty and green.

## Reviewer Reject Signals

- The audit relies only on the post-713 directory listing or current `HEAD`
  without selecting and comparing the complete idea-713 history range.
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
