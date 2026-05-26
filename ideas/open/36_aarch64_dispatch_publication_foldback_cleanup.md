# AArch64 Dispatch Publication Fold-Back Cleanup

## Goal

Fold small target-local dispatch and publication helper declarations back into
their reference-style AArch64 owners without changing prepared publication
authority.

## Why This Exists

The layout classification marked `dispatch_diagnostics.*` and
`dispatch_publication_common.hpp` as mechanical fold-back work. Their logic is
target-local diagnostics and private publication helper declarations around
already-prepared facts. They do not justify separate owner families, and they
must not be confused with semantic edge-publication migration.

## In Scope

- Fold these target-local helper families into their owner files:
  - `src/backend/mir/aarch64/codegen/dispatch_diagnostics.cpp`
  - `src/backend/mir/aarch64/codegen/dispatch_diagnostics.hpp`
  - `src/backend/mir/aarch64/codegen/dispatch_publication_common.hpp`
- Use `dispatch.cpp` / `dispatch.hpp` for dispatch diagnostics ownership.
- Use `dispatch_publication.cpp` / `dispatch_publication.hpp` for private
  publication declarations and helpers.
- Update includes and build metadata only as needed for the fold-back.
- Preserve prepared edge-publication and value-home lookup as the semantic
  authority consumed by AArch64.

## Out Of Scope

- Reworking `dispatch_edge_copies.*`, `dispatch_lookup.*`,
  `dispatch_producers.*`, `dispatch_publication.*`, or
  `dispatch_value_materialization.*` beyond include/API fallout.
- Moving edge-publication, value-flow, or prepared-home authority into AArch64
  dispatch.
- Broad dispatch slimming or orchestration rewrites.
- Changing diagnostic meaning or weakening fail-closed unsupported paths.
- Calls bridge consolidation, which belongs to the calls fold-back idea.

## Acceptance Criteria

- Dispatch diagnostics and publication-common declarations no longer live in
  standalone extra owner files.
- AArch64 dispatch and publication behavior remains unchanged except for owner
  placement.
- Build metadata no longer references removed helper translation units.
- Validation covers focused AArch64 dispatch/publication behavior and a build
  or backend test bucket sufficient for include and linkage changes.

## Reviewer Reject Signals

- The patch uses fold-back as a reason to rediscover edge-copy, publication,
  or value-home facts in AArch64 instead of consuming prepared records.
- The patch changes diagnostics by deleting unsupported cases, downgrading
  tests, or weakening contracts without explicit user approval.
- The patch special-cases one branch label, block id, value id, or fixture to
  make a narrow dispatch test pass.
- The patch moves unrelated calls, memory, comparison, or prologue helpers in
  the same slice.
- The patch keeps the same standalone ownership behind new
  `*_common`, `*_bridge`, or `*_helpers` files.
