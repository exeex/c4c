# AArch64 Calls File Consolidation

## Goal

Consolidate the AArch64 calls file family after prepared authority and
emission-only boundaries are stable.

## Why This Exists

File merging before authority cleanup just creates larger files. Once call
source selection, preservation, republication, and printing ownership have been
moved to proper layers, the remaining AArch64 calls code should be small enough
to consolidate safely.

## In Scope

- Merge remaining small AArch64 calls files with clear emission-only roles.
- Delete retired headers and translation units.
- Update build metadata and includes.
- Keep `calls.hpp` minimal and target-emission oriented.
- Run focused and broader call/backend validation.

## Out Of Scope

- Moving new semantic authority during this consolidation.
- Dispatch cleanup.
- Rewriting ABI classification.

## Acceptance Criteria

- The calls file family is materially smaller and easier to scan.
- Consolidation does not change behavior.
- Build metadata has no stale calls entries.
- Validation proves no regression from the file move/deletion itself.

## Reviewer Reject Signals

- A patch combines files while preserving the same hidden semantic debt.
- A patch changes behavior without separating it into an earlier authority
  cleanup idea.
- A patch leaves dead includes or build entries.
- A patch treats line-count reduction as success without proof.
