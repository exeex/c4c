# Closed Idea Metadata Cleanup After Prepared Contracts

Status: Open
Type: Lifecycle metadata cleanup idea

## Goal

Clean up stale lifecycle metadata left by the prepared contract normalization
round, starting with closed idea files whose in-file status disagrees with
their archive location.

## Why This Exists

The prepared contract round closed ideas 413 through 418 and removed active
`plan.md` / `todo.md` state. A follow-up review found that
`ideas/closed/414_typed_prepared_call_argument_contracts.md` still says
`Status: Open` even though it has a closure note and lives under
`ideas/closed/`.

That mismatch is small, but it is the kind of stale lifecycle signal that can
mislead later agents when they decide whether an idea is still active,
blocked, or complete.

## In Scope

- Audit closed prepared-contract idea files 413 through 418 for stale status
  headers, outdated `ideas/open/...` self-references, or missing closure
  discoverability.
- Fix metadata-only inconsistencies in closed idea files.
- Keep the edits limited to lifecycle metadata and short clarification notes.
- Verify the worktree remains cleanly formatted with `git diff --check`.

## Out Of Scope

- Changing implementation code.
- Reopening or reordering prepared-contract ideas.
- Rewriting closure history or proof claims.
- Running RV64 gcc_torture or changing its expectations.
- Broad docs rewrites unrelated to stale lifecycle metadata.

## Acceptance Criteria

- Closed idea files 413 through 418 no longer contain a stale `Status: Open`
  header.
- Any corrected self-reference still preserves historical source intent and
  closure notes.
- `git diff --check` passes.
- No default CTest run is required for metadata-only edits.

## Reviewer Reject Signals

- Reject edits that change technical scope or proof claims while presented as
  metadata cleanup.
- Reject moving ideas between open and closed without a real lifecycle
  decision.
- Reject deleting closure notes or regression evidence.
- Reject broad formatting churn across unrelated idea files.
- Reject any implementation or test expectation change in this cleanup slice.
