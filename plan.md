# Post-Legacy BIR Shell Bootstrap Runbook

Status: Active
Source Idea: ideas/open/730_post_legacy_bir_shell_bootstrap.md

## Purpose

Restore a buildable backend around a minimal new BIR shell and two direct
interface seams, while keeping all legacy BIR/prealloc code reference-only.

## Goal

Make CMake generation and builds succeed without legacy paths, migrate active
LIR-to-BIR code to an explicit empty BIR view, add an empty-view BIR-to-MIR
consumer, and retain only focused tests of those two interfaces.

## Core Rule

Never compile, copy, rename, wrap, or re-export `src/backend/legacy` code.  The
new shell begins with explicit safe empty behavior and does not claim semantic
target-codegen support.

## Read First

- `ideas/open/730_post_legacy_bir_shell_bootstrap.md`
- `src/backend/bir/lir_to_bir/`
- `src/backend/legacy/` as read-only historical reference
- backend production and test CMake files
- current MIR-facing backend entry points

## Current Targets

- Obsolete backend test registrations blocking CMake generation.
- Minimal new BIR schema and published empty view.
- Active `src/backend/bir/lir_to_bir` migration.
- Minimal BIR-to-MIR empty-view consumer.
- Direct LIR-to-new-BIR and new-BIR-to-MIR tests.
- Dedicated support/build cleanup and final broader validation.

## Non-Goals

- Do not compile or transplant legacy BIR/prealloc.
- Do not restore non-empty lowering, target emission, objects, or runtime
  semantics.
- Do not preserve internal or downstream tests outside the two interfaces.
- Do not disguise silent data loss as successful code generation.

## Execution Rules

- Configure and build after each seam; let the next failure identify the next
  necessary minimal contract.
- Keep new schema and consumer APIs smaller than the legacy surface.
- Make empty and unsupported behavior explicit and testable.
- Delete tests from assertion-level classification, including their dedicated
  support only when no retained interface test uses it.
- Run focused interface proof after shared changes and broader CTest at the
  final checkpoint.

## Step 1: Restore CMake generation without legacy sources

Goal: Remove obsolete test graph edges so configuration exposes the first real
production build seam.

Primary target: backend test CMake registrations.

Concrete actions:

- Remove obsolete internal/legacy backend test targets referencing missing or
  quarantined sources, beginning with `backend_prepare_phi_materialize_test`
  and `backend_lir_to_bir_notes_test`.
- Remove only their exclusively dedicated registration/support entries in this
  packet.
- Verify no production or test target names `src/backend/legacy/**`.
- Run fresh CMake generation, then build the default preset immediately.
- Record the first compiler or linker failure as the next seam.

Completion check:

- CMake generation succeeds without legacy sources and the first true build
  seam is captured with a reproducible command and diagnostic.

## Step 2: Define the minimal new BIR schema and empty view

Goal: Provide the smallest non-legacy BIR representation needed for interface
bootstrap.

Primary target: new active files under `src/backend/bir/`, outside
`lir_to_bir/` and `legacy/`.

Concrete actions:

- Define explicit empty module/view invariants and a minimal publication API.
- Add only fields and operations required by the observed build seam and empty
  interface proof.
- Fail clearly on unsupported non-empty operations.
- Build the affected production targets and add focused empty-view proof.

Completion check:

- The new schema/view builds independently of legacy code and its empty and
  unsupported states are observable and safe.

## Step 3: Migrate LIR-to-BIR onto the new shell

Goal: Keep the active LIR-to-BIR source family while removing its dependency on
the old representation.

Primary target: `src/backend/bir/lir_to_bir/`.

Concrete actions:

- Adapt the retained entry point to publish the new empty BIR view for the
  explicitly supported bootstrap path.
- Do not reintroduce legacy builder, route, preallocation, ID, or lookup APIs.
- Reject unsupported meaningful/non-empty lowering rather than silently
  discarding it.
- Add or retain only direct LIR-to-new-BIR boundary tests.

Completion check:

- LIR-to-BIR builds against the new shell and focused tests distinguish valid
  empty publication from safe rejection of unsupported input.

## Step 4: Add the minimal BIR-to-MIR empty-view consumer

Goal: Establish the second direct interface without rebuilding downstream
backend semantics.

Concrete actions:

- Define a minimal consumer entry accepting the new BIR view.
- Return explicit empty MIR or a clear safe bootstrap outcome for an empty
  input view.
- Reject unsupported non-empty use without pretending target codegen works.
- Add focused direct new-BIR-to-MIR tests.

Completion check:

- The empty view crosses the BIR-to-MIR seam under focused proof, with no
  legacy dependency or implied non-empty target support.

## Step 5: Retire remaining obsolete tests and dedicated support

Goal: Align the backend test graph with the two-interface-only policy.

Concrete actions:

- Remove route, prepare/prealloc, dump/printer/lookup/ID, target
  lowering/emission, object, and runtime backend tests outside direct
  LIR-to-new-BIR or new-BIR-to-MIR contracts.
- Remove their exclusively dedicated fixtures, helpers, expectations, targets,
  options, gates, and registrations.
- Preserve shared support only when a retained boundary test still uses it.
- Configure, build, and run both focused interface test groups after each
  shared-support batch.

Completion check:

- Only direct tests of the two new interfaces remain and no stale dedicated
  support or registration references retired tests.

## Step 6: Validate the bootstrap surface broadly

Goal: Prove repository health and the explicit limits of the new shell.

Concrete actions:

- Run clean preset generation and the default build.
- Run non-empty focused LIR-to-new-BIR and new-BIR-to-MIR test discovery and
  execution.
- Run broader CTest with output-on-failure.
- Confirm build metadata contains no `src/backend/legacy` path.
- Confirm empty behavior is explicit and unsupported non-empty paths fail
  safely rather than claiming codegen success.

Completion check:

- Configure, build, focused interface proof, and broader CTest are reviewed;
  legacy stays reference-only and the bootstrap makes no false semantic-output
  claim.
