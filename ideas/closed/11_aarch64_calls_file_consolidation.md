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

## Closure Note

Closed after Step 5 acceptance validation. The consolidation reduced the
remaining AArch64 calls file family by retiring the stale operand-adapter
translation unit into the active common calls owner, simplifying public calls
header surface, and verifying no retired calls entries or includes remained.

Acceptance evidence:

- Reviewer report `review/aarch64_calls_consolidation_step5_acceptance_review.md`
  found no blocking route, behavior-preservation, metadata, or header-surface
  issues.
- Close-time backend regression guard compared canonical `test_before.log` and
  `test_after.log`; both had 162/162 tests passed, no new failures, and
  non-decreasing pass count.
- Accepted full-suite baseline `test_baseline.log` at `f33116d71` recorded
  3410/3410 tests passed.

## Reviewer Reject Signals

- A patch combines files while preserving the same hidden semantic debt.
- A patch changes behavior without separating it into an earlier authority
  cleanup idea.
- A patch leaves dead includes or build entries.
- A patch treats line-count reduction as success without proof.
