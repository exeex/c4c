# Post-Legacy BIR Shell Bootstrap

Status: Open
Type: backend interface bootstrap and build restoration

## Intent

Restore a buildable backend around a new minimal BIR shell without compiling or
reimporting the legacy BIR and preallocation implementation.  Preserve the
`src/backend/bir/lir_to_bir` source family, migrate it onto the new shell, and
publish an explicit empty BIR view through a minimal BIR-to-MIR consumer seam.

This is bootstrap work, not semantic target-codegen restoration.  Empty
BIR/MIR behavior must be explicit and fail safely wherever non-empty backend
output is still unsupported.

## Why This Exists

The old BIR and preallocation implementation now lives under
`src/backend/legacy`, while active test registrations still name removed
production sources such as `src/backend/bir/bir.cpp`.  CMake generation fails
before the repository can reveal the next real interface seam.  A small new
schema and consumer path are needed so the retained LIR-to-BIR and BIR-to-MIR
contracts can compile independently of the quarantined implementation.

## Core Architecture Contract

- `src/backend/legacy/**` is reference-only and forbidden from production and
  test build graphs.
- `src/backend/bir/lir_to_bir/**` remains the active LIR-to-BIR source family,
  but must migrate to the new schema instead of importing, copying, renaming,
  or wrapping legacy BIR APIs.
- The new BIR layer begins as the smallest schema/view capable of representing
  and publishing an explicitly empty module or equivalent top-level view.
- The new BIR-to-MIR seam consumes that empty view and produces an explicitly
  empty result or a clear safe unsupported outcome.  It must not imply that
  target lowering, emission, object generation, or runtime execution works.
- Tests survive only when they directly specify LIR-to-new-BIR or
  new-BIR-to-MIR.  Other backend tests and their dedicated support and
  registrations are removed.

## Required Sequence

1. Restore CMake generation first by removing obsolete legacy/internal backend
   test targets and registrations that reference missing or quarantined
   sources.  Do not add legacy paths to make generation pass.
2. Build immediately after generation succeeds.  Treat the first compiler or
   linker failure as the next true production seam and record it before adding
   new APIs.
3. Introduce the minimal new BIR schema/view with explicit empty-state
   invariants and no copied legacy representation surface.
4. Migrate the retained `lir_to_bir` source family to construct or publish the
   new empty shell for the supported bootstrap input path.
5. Introduce the minimal BIR-to-MIR consumer entry that accepts the empty view
   and returns explicit empty/safe bootstrap behavior.
6. Add or retain only focused direct-interface tests, remove remaining obsolete
   tests and dedicated fixtures/helpers/expectations/registrations, and finish
   with focused interface proof plus broader CTest.

## In Scope

- CMake/test registration cleanup required to generate and expose the next
  active backend seam.
- A minimal non-legacy BIR schema and read-only/publication view.
- Migration of `src/backend/bir/lir_to_bir` to the new shell.
- A minimal new BIR-to-MIR consumer entry for an empty view.
- Direct LIR-to-new-BIR and new-BIR-to-MIR boundary tests.
- Removal of obsolete backend tests and their exclusively dedicated support,
  build targets, and registrations.
- Incremental configure/build proof and final broader CTest.

## Out Of Scope

- Compiling anything under `src/backend/legacy`.
- Copying, renaming, wrapping, or progressively recreating legacy BIR or
  preallocation APIs as the new shell.
- Restoring non-empty target lowering, instruction selection, register
  allocation, emission, object generation, or runtime semantics.
- Keeping route, prepare/prealloc, internal dump/printer/lookup/ID, target, or
  runtime tests outside the two direct interface contracts.
- Weakening a retained boundary expectation or relabeling an internal test as
  a boundary test.

## Validation Ladder

For each implementation seam:

1. run CMake generation when build graph inputs change
2. build the affected target or default preset to expose the next seam
3. run focused direct-interface tests as soon as they exist
4. run broader CTest after obsolete test/support cleanup is complete

The initial accepted proof may produce no semantic backend output, but empty
behavior must be observable, intentional, and safe.

## Acceptance Criteria

- CMake generation succeeds without any source or target under
  `src/backend/legacy` entering a production or test build graph.
- Obsolete registrations referencing removed BIR sources are gone, including
  the known `backend_prepare_phi_materialize_test` and
  `backend_lir_to_bir_notes_test` failures unless replaced by genuinely direct
  new-interface proof.
- A minimal new BIR schema/view represents and publishes an explicit empty
  state without legacy API transplantation.
- The active `lir_to_bir` source family builds against the new shell.
- A minimal BIR-to-MIR entry accepts the empty view and returns explicit empty
  or safe unsupported behavior without claiming target codegen support.
- Only direct LIR-to-new-BIR and new-BIR-to-MIR backend tests remain; obsolete
  tests and their dedicated fixtures, helpers, expectations, targets, and
  registrations are removed.
- Incremental configure/build proof is recorded after each seam, both focused
  interface contracts are tested, and the final broader CTest result is
  reviewed under the new bootstrap surface.

## Reviewer Reject Signals

- Any production or test target compiles a path under `src/backend/legacy`.
- Legacy BIR or preallocation types, APIs, layouts, helpers, or route machinery
  are copied, renamed, wrapped, symlinked, or re-exported as the new shell.
- CMake generation is repaired by adding missing legacy sources rather than
  deleting obsolete non-boundary test targets and registrations.
- The implementation grows non-empty target semantics before the minimal empty
  schema and two interface seams have independent proof.
- Empty behavior silently drops meaningful input or pretends successful target
  codegen; unsupported non-empty use must fail clearly and safely.
- Route, prepared/prealloc, dump/printer/lookup/ID, target lowering/emission,
  object, or runtime tests remain without direct assertion-level proof of one
  retained interface.
- A retained interface expectation is weakened or an internal test is merely
  relabeled to survive cleanup.
- Only the known CMake error is patched while the next build seam is left
  unobserved, or implementation batches omit their matching build proof.
