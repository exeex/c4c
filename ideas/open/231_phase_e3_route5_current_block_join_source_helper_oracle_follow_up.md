# 231 Phase E3 Route 5 current-block join-source helper-oracle follow-up

## Goal

Augment or replace one Route 5 current-block join-source helper-oracle row for
`PreparedCurrentBlockJoinParallelCopySourceFact` with Route 5 diagnostic
metadata after agreement with prepared edge/join semantics.

## Why This Exists

The Route 5 closure proved diagnostic metadata for one current-block
join-source helper-oracle success row after prepared edge/join agreement. Phase
E3 accepted only that helper-oracle row. The adjacent prepared printer row is
retained/deferred unless a future separate idea names it exactly.

## In Scope

- One current-block join-source helper-oracle row.
- Positive Route 5 current-block join-source metadata after prepared edge/join
  agreement.
- Absent, invalid, duplicate/conflicting, memory-source, mismatch, unsupported,
  branch/parallel-copy, and prepared fallback cases.
- Proof that prepared-printer behavior, wrapper output, helper-oracle strings,
  expected strings, and nearby same-feature cases remain stable.

## Out Of Scope

- Prepared-printer migration or an either/or helper-oracle versus prepared
  printer route.
- Broad edge publication, move-bundle, parallel-copy, branch, wrapper, or
  prepared-printer replacement.
- Expected-string rewrites, baseline refreshes, helper renames, unsupported
  downgrades, or weakened fallback/oracle behavior.
- Target policy, emitted-output policy, draft 155, E5, or aggregate prepared
  retirement.

## Acceptance Criteria

- The helper-oracle success row uses Route 5 metadata only after prepared
  edge/join agreement.
- Absent, invalid, duplicate/conflicting, memory-source, mismatch, unsupported,
  branch/parallel-copy, and prepared-only paths retain prepared authority.
- Prepared-printer and wrapper behavior remain byte-stable and explicitly
  retained.
- Proof covers nearby same-feature join-source cases without promoting printer
  ownership.
- No edge-publication, move-bundle, prepared-printer, wrapper, baseline, or
  expected-string migration is part of accepted progress.

## Reviewer Reject Signals

- The idea or implementation turns into an either/or prepared-printer route
  instead of the single current-block join-source helper-oracle row.
- It special-cases one `PreparedCurrentBlockJoinParallelCopySourceFact` fixture
  without proving Route 5 metadata agreement.
- It weakens absent, invalid, duplicate/conflicting, memory-source, mismatch,
  unsupported, branch/parallel-copy, or prepared fallback cases.
- It changes prepared-printer output, wrapper output, expected strings,
  baselines, helper names, or supported/unsupported contracts to claim progress.
- It broadens into edge-publication, move-bundle, parallel-copy, or target
  policy migration.
