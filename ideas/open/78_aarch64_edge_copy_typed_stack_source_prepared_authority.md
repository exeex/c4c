# AArch64 Edge-Copy Typed Stack-Source Prepared Authority

## Goal

Make the AArch64 edge-copy publication owner consume
`PreparedTypedStackSourcePublication` for same-width I32 stack-source
publications before target-local load or copy emission.

## Why This Exists

Active idea `ideas/open/69_aarch64_call_publication_prepared_authority_cleanup.md`
found that `dispatch_publication.cpp` already consumes prepared store-source
publication plans and does not hold the `PreparedEdgePublication*` required by
`prepare_same_width_i32_stack_source_publication`. The viable owner appears to
be `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`, which is outside
idea 69's owned file list.

This follow-up keeps the Step 4 typed stack-source migration from expanding the
active runbook across ownership boundaries while preserving the durable cleanup
intent.

## In Scope

- Inspect `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp` call sites
  that already receive `PreparedEdgePublication` or
  `PreparedEdgePublication*`.
- Route same-width I32 stack-source publication through
  `prepare_same_width_i32_stack_source_publication` where that prepared fact
  matches the edge-copy publication decision.
- Preserve AArch64 load, register-view, extension, copy instruction, and
  machine-record construction locally.
- Add or adjust narrow proof for the touched typed stack-source edge-copy
  publication path.

## Out Of Scope

- Threading `PreparedEdgePublication` into `dispatch_publication.cpp` solely to
  satisfy idea 69 Step 4.
- Rewriting publication ordering, edge-copy scheduling, or the shared
  publication-plan model.
- Moving AArch64 instruction selection, register spelling, or address rendering
  into shared prealloc code.
- Weakening supported-path expectations, unsupported markers, diagnostics, or
  tests to claim progress.

## Acceptance Criteria

- The edge-copy typed stack-source path consumes
  `PreparedTypedStackSourcePublication` when the prepared fact is available.
- The implementation leaves only target-local load/copy/register rendering in
  AArch64 code.
- Focused proof covers the same-width I32 stack-source edge-copy publication
  path touched by the migration.
- Any remaining unsupported or missing-authority cases are recorded as
  explicit blockers rather than hidden behind generic fallback logic.

## Reviewer Reject Signals

- The diff adds a named testcase shortcut or publication-shape special case
  instead of consuming `PreparedTypedStackSourcePublication`.
- The route weakens or rewrites test expectations, diagnostics, or supported
  markers without explicit user approval.
- A helper rename, wrapper, or fallback still locally re-derives the same-width
  I32 stack-source authority that the prepared fact already provides.
- The implementation moves AArch64 load mnemonics, register spelling,
  extension policy rendering, or machine-record construction into shared
  prealloc code.
- The route broadens into publication ordering, dispatch-family contraction, or
  unrelated edge-copy rewrites instead of the typed stack-source authority
  cleanup.
