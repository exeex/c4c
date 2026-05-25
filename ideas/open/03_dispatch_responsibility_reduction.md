# Dispatch Responsibility Reduction

## Goal

Reduce AArch64 `dispatch*` to block traversal and instruction routing, moving
value materialization, publication, edge copies, and same-block producer logic
behind clearer shared or target-local ownership boundaries.

## Why This Exists

The current `dispatch*` family is not just dispatch. It mixes prepared block
traversal with value-home lookup, materialization, publication, edge-copy
lowering, producer recovery, diagnostics, and call-specific glue. That makes it
hard to understand what dispatch owns and encourages more compatibility logic
to accumulate under a vague name.

## In Scope

- Define the intended responsibility of `dispatch.cpp` as prepared block
  traversal and instruction/terminator routing.
- Identify materialization and publication helpers that should move behind
  named owner surfaces.
- Separate edge-copy or join-value handling from ordinary instruction dispatch
  where the current code supports that split.
- Remove call-specific materialization paths made unnecessary by shared call
  plans.
- Preserve the compiled target MIR / machine-node route documented in
  `src/backend/mir/aarch64/codegen/README.md`.

## Out Of Scope

- Starting this work before the calls cleanup has established shared call-plan
  authority.
- Replacing the whole AArch64 codegen pipeline.
- Reintroducing text-first emission or parsing printed assembly as an internal
  contract.
- Mechanical renames that keep the same hidden responsibility pile.

## Acceptance Criteria

- `dispatch.cpp` reads as traversal/routing rather than a mixed lowering
  coordinator.
- Value materialization, publication, edge copies, and producer lookup have
  explicit owner names and interfaces.
- Remaining call-specific dispatch glue is either removed or justified by the
  shared call-plan boundary.
- Focused backend tests plus build proof pass after each split.
- Documentation or comments describe the new boundary without creating
  markdown shards inside `src/backend/mir/aarch64/codegen/`.

## Reviewer Reject Signals

- A patch only renames `dispatch*` files without reducing responsibility.
- A patch moves dispatch behavior into another vague bucket with no clearer
  owner.
- A patch expands target-local compatibility matching for narrow prepared BIR
  shapes instead of creating semantic materialization or publication rules.
- A patch breaks the compiled machine-node route or revives text-emission as
  the internal handoff.
- A patch uses call cleanup as a reason to silently change non-call lowering
  behavior.
